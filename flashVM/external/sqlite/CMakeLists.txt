# The pinned amalgamation is compiled locally; never search for system SQLite.
add_library(fvm_sqlite STATIC upstream/sqlite3.c)
target_include_directories(fvm_sqlite PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/upstream
  ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(fvm_sqlite PRIVATE SQLITE_THREADSAFE=1
  SQLITE_OMIT_LOAD_EXTENSION=1 SQLITE_DQS=0 SQLITE_DEFAULT_MEMSTATUS=0)
if(NOT WIN32)
  target_compile_definitions(fvm_sqlite PRIVATE _POSIX_C_SOURCE=200809L _DEFAULT_SOURCE=1 _GNU_SOURCE=1)
  target_link_libraries(fvm_sqlite PUBLIC Threads::Threads ${CMAKE_DL_LIBS} m)
endif()
