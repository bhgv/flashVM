/* C89: optional SQLite C adapter, with an in-memory database. */
#include "flashvm/sqlite.h"
#include <stdio.h>
int main(void) {
 FVM_SQLite *db=0;long result=0;
 if(fvm_sqlite_open(":memory:",&db)!=FVM_OK) return 1;
 if(fvm_sqlite_exec(db,"CREATE TABLE scores (points INTEGER); INSERT INTO scores VALUES (7);")!=FVM_OK) return 2;
 if(fvm_sqlite_query_int(db,"SELECT points FROM scores",&result)!=FVM_OK) return 3;
 printf("SQLite score: %ld\n",result);
 fvm_sqlite_close(db);return result==7?0:4;
}
