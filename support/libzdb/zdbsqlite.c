
#define SQLITE_API static
#define SQLITE_EXTERN static
#include "sqlite3.c"

#undef T
#include "SQLiteConnection.c"
#undef T
#include "SQLitePreparedStatement.c"
#undef T
#include "SQLiteResultSet.c"
