#include "privilege.h"
#include "webpa_adapter.h"
#include "cap.h"

cap_user appcaps;
bool blocklist_ret = false;

void drop_root_privilege()
{
   appcaps.caps = NULL;
   appcaps.user_name = NULL;
   blocklist_ret = isBlocklisted();
   if(blocklist_ret)
   {
       WalInfo("NonRoot feature is disabled\n");
   }
   else
   {
       WalInfo("NonRoot feature is enabled, dropping root privileges for webpa process\n");
       init_capability();
       drop_root_caps(&appcaps);
       update_process_caps(&appcaps);
       read_capability(&appcaps);
       clear_caps(&appcaps);
   }
}
