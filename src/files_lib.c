#include "cf3.defs.h"
#include "cf3.extern.h"

static struct Item *NextItem(struct Item *ip);
static int ItemListsEqual(struct Item *list1, struct Item *list2,int report,struct Attributes a,struct Promise *pp);

/*********************************************************************/

void PurgeItemList(struct Item **list,char *name)

{ struct Item *ip,*copy = NULL;
  struct stat sb;

CopyList(&copy,*list);
  
for (ip = copy; ip != NULL; ip=ip->next)
   {
   if (cfstat(ip->name,&sb) == -1)
      {
      CfOut(cf_verbose,""," -> Purging file \"%s\" from %s list as it no longer exists",ip->name,name);
      DeleteItemLiteral(list,ip->name);
      }
   }

DeleteItemList(copy);
}

/*********************************************************************/

int RawSaveItemList(struct Item *liststart,char *file)

{ struct Item *ip;
  char new[CF_BUFSIZE],backup[CF_BUFSIZE];
  FILE *fp;
  time_t STAMPNOW;
  STAMPNOW = time((time_t *)NULL);

strcpy(new,file);
strcat(new,CF_EDITED);

strcpy(backup,file);
strcat(backup,CF_SAVED);

unlink(new); /* Just in case of races */ 
 
if ((fp = fopen(new,"w")) == NULL)
   {
   CfOut(cf_error,"fopen","Couldn't write file %s\n",new);
   return false;
   }

for (ip = liststart; ip != NULL; ip=ip->next)
   {
   fprintf(fp,"%s\n",ip->name);
   }

if (fclose(fp) == -1)
   {
   CfOut(cf_error,"fclose","Unable to close file while writing");
   return false;
   }

if (cf_rename(new,file) == -1)
   {
   CfOut(cf_inform,"cf_rename","Error while renaming %s\n",file);
   return false;
   }       

return true;
}

/*********************************************************************/

int CompareToFile(struct Item *liststart,char *file,struct Attributes a,struct Promise *pp)

/* returns true if file on disk is identical to file in memory */

{
  struct stat statbuf;
  struct Item *cmplist = NULL;

Debug("CompareToFile(%s)\n",file);

if (cfstat(file,&statbuf) == -1)
   {
   return false;
   }

if (liststart == NULL && statbuf.st_size == 0)
   {
   return true;
   }

if (liststart == NULL)
   {
   return false;
   }

if (!LoadFileAsItemList(&cmplist,file,a,pp))
   {
   return false;
   }

if (!ItemListsEqual(cmplist,liststart,(a.transaction.action == cfa_warn),a,pp))
   {
   DeleteItemList(cmplist);
   return false;
   }

DeleteItemList(cmplist);
return (true);
}

/*********************************************************************/

static int ItemListsEqual(struct Item *list1,struct Item *list2,int warnings,struct Attributes a,struct Promise *pp)

// Some complex logic here to enable warnings of diffs to be given
    
{ struct Item *ip1, *ip2;
  int retval = true;
 
ip1 = list1;
ip2 = list2;

while (true)
   {
   if ((ip1 == NULL) && (ip2 == NULL))
      {
      return retval;
      }

   if ((ip1 == NULL) || (ip2 == NULL))
      {
      if (warnings)
         {
         if (ip1 == list1 || ip2 == list2)
            {
            cfPS(cf_error,CF_WARN,"",pp,a," ! File content wants to change from from/to full/empty but only a warning promised");
            }
         else
            {
            if (ip1 != NULL)
               {
               cfPS(cf_error,CF_WARN,"",pp,a," ! edit_line change warning promised: (remove) %s",ip1->name);
               }

            if (ip2 != NULL)
               {               
               cfPS(cf_error,CF_WARN,"",pp,a," ! edit_line change warning promised: (add) %s",ip2->name);
               }
            }
         }

      if (warnings)
         {
         if (ip1 || ip2)
            {
            retval = false;
            ip1 = NextItem(ip1);
            ip2 = NextItem(ip2);
            continue;
            }
         }

      return false;
      }

   if (strcmp(ip1->name,ip2->name) != 0)
      {
      if (!warnings)
         {
         // No need to wait
         return false;
         }
      else
         {
         // If we want to see warnings, we need to scan the whole file

         cfPS(cf_error,CF_WARN,"",pp,a," ! edit_line warning promised: - %s",ip1->name);
         cfPS(cf_error,CF_WARN,"",pp,a," ! edit_line warning promised: + %s",ip2->name);
         retval = false;
         }
      }

   ip1 = NextItem(ip1);
   ip2 = NextItem(ip2);
   }

return retval;
}

/*********************************************************************/
/* helpers                                                           */
/*********************************************************************/

struct Item *NextItem(struct Item *ip)

{
if (ip)
   {
   return ip->next;
   }
else
   {
   return NULL;
   }
}