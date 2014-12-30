#include "RS-MySQL.h"

static MySQLDriver* dbManager = NULL;
MySQLDriver* rmysql_driver() {
  if (!dbManager) error("Corrupt MySQL handle");
  return dbManager;
}

SEXP rmysql_driver_valid() {
  if(!dbManager || !dbManager->connections) {
    return ScalarLogical(FALSE);
  } else {
    return ScalarLogical(TRUE);
  }
}

SEXP rmysql_driver_init(SEXP max_con_, SEXP fetch_default_rec_) {
  SEXP mgrHandle = ScalarInteger(0);
  if (dbManager) return mgrHandle;

  int max_con = asInteger(max_con_),
      fetch_default_rec = asInteger(fetch_default_rec_);

  int counter = 0;
  MySQLDriver* mgr = (MySQLDriver*) malloc(sizeof(MySQLDriver));
  if (!mgr)
    error("Could not allocate memory for the MySQL driver");

  /* Ok, we're here to expand number of connections, etc.*/
  mgr->managerId = 0;
  mgr->connections = calloc(max_con, sizeof(RS_DBI_connection));
  if (!mgr->connections) {
    free(mgr);
    error("Could not allocate memory for connections");
  }

  mgr->connectionIds = calloc(max_con, sizeof(int));
  if (!mgr->connectionIds){
    free(mgr->connections);
    free(mgr);
    error("Could not allocation memory for connection Ids");
  }
  mgr->counter = counter;
  mgr->length = max_con;
  mgr->num_con = (int) 0;
  mgr->fetch_default_rec = fetch_default_rec;

  for(int i = 0; i < max_con; i++){
    mgr->connectionIds[i] = -1;
    mgr->connections[i] = (RS_DBI_connection *) NULL;
  }

  dbManager = mgr;

  return mgrHandle;
}

SEXP rmysql_driver_close() {
  MySQLDriver *mgr = rmysql_driver();

  if(mgr->num_con)
    error("Open connections -- close them first");

  if(mgr->connections) {
    free(mgr->connections);
    mgr->connections = (RS_DBI_connection **) NULL;
  }

  if(mgr->connectionIds) {
    free(mgr->connectionIds);
    mgr->connectionIds = (int *) NULL;
  }

  return ScalarLogical(TRUE);
}

SEXP         /* named list */
RS_DBI_managerInfo(SEXP mgrHandle)
{

  MySQLDriver *mgr;
  SEXP output;
  int  i, num_con;
  int n = (int) 7;
  char *mgrDesc[] = {"connectionIds", "fetch_default_rec","managerId",
    "length", "num_con", "counter", "clientVersion"};
  SEXPTYPE mgrType[] = {INTSXP, INTSXP, INTSXP,
    INTSXP, INTSXP, INTSXP,
    STRSXP};
  int  mgrLen[]  = {1, 1, 1, 1, 1, 1, 1};

  mgr = rmysql_driver();
  num_con = (int) mgr->num_con;
  mgrLen[0] = num_con;

  output = RS_DBI_createNamedList(mgrDesc, mgrType, mgrLen, n);

  for(i = 0; i < num_con; i++)
    LST_INT_EL(output,0,i) = (int) mgr->connectionIds[i];

  LST_INT_EL(output,1,0) = (int) mgr->fetch_default_rec;
  LST_INT_EL(output,2,0) = (int) mgr->managerId;
  LST_INT_EL(output,3,0) = (int) mgr->length;
  LST_INT_EL(output,4,0) = (int) mgr->num_con;
  LST_INT_EL(output,5,0) = (int) mgr->counter;
  SET_LST_CHR_EL(output,6,0,C_S_CPY("NA"));   /* client versions? */

return output;
}




SEXP
  RS_MySQL_managerInfo(SEXP mgrHandle)
  {
    MySQLDriver *mgr;
    SEXP output;
    int i, num_con, max_con, *cons, ncon;
    int j, n = 8;
    char *mgrDesc[] = {"drvName",   "connectionIds", "fetch_default_rec",
      "managerId", "length",        "num_con",
      "counter",   "clientVersion"};
    SEXPTYPE mgrType[] = {STRSXP, INTSXP, INTSXP,
      INTSXP,   INTSXP, INTSXP,
      INTSXP,   STRSXP};
    int  mgrLen[]  = {1, 1, 1, 1, 1, 1, 1, 1};

    mgr = rmysql_driver();
    if(!mgr)
      RS_DBI_errorMessage("driver not loaded yet", RS_DBI_ERROR);
    num_con = (int) mgr->num_con;
    max_con = (int) mgr->length;
    mgrLen[1] = num_con;

    output = RS_DBI_createNamedList(mgrDesc, mgrType, mgrLen, n);

    j = (int) 0;
    SET_VECTOR_ELT(output, j++, mkChar("RMySQL"));

    cons = (int *) S_alloc((long)max_con, (int)sizeof(int));
    ncon = RS_DBI_listEntries(mgr->connectionIds, mgr->length, cons);
    if(ncon != num_con){
      RS_DBI_errorMessage(
        "internal error: corrupt RS_DBI connection table",
        RS_DBI_ERROR);
    }

    for(i = 0; i < num_con; i++)
      LST_INT_EL(output, j, i) = cons[i];
    j++;
    LST_INT_EL(output,j++,0) = mgr->fetch_default_rec;
    LST_INT_EL(output,j++,0) = mgr->managerId;
    LST_INT_EL(output,j++,0) = mgr->length;
    LST_INT_EL(output,j++,0) = mgr->num_con;
    LST_INT_EL(output,j++,0) = mgr->counter;
    SET_LST_CHR_EL(output,j++,0,C_S_CPY(mysql_get_client_info()));

    return output;
  }
