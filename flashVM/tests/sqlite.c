#include "flashvm/sqlite.h"
int main(void) {
 FVM_SQLite *db=0;long n=0;
 if(fvm_sqlite_open(":memory:",&db)!=FVM_OK) return 1;
 if(fvm_sqlite_exec(db,"CREATE TABLE t (v INTEGER); INSERT INTO t VALUES (42);")!=FVM_OK) return 2;
 if(fvm_sqlite_query_int(db,"SELECT v FROM t",&n)!=FVM_OK || n!=42) return 3;
 fvm_sqlite_close(db);return 0;
}
