#ifndef MARIADB_CLIENT_MODES
    #define MARIADB_CLIENT_MODES

    /* Banderas individuales de MariaDB Connector/C */
    #define MARIADB_FLAG_LOCAL_FILES      128      /* 0x00080: Permite LOAD DATA LOCAL INFILE */
    #define MARIADB_FLAG_MULTI_STATEMENTS 65536    /* 0x10000: Permite múltiples queries separadas por ';' */

    /* Modos permitidos para el parámetro client_mode */
    #define MARIADB_CLIENT_DEFAULT_MODE     MARIADB_FLAG_LOCAL_FILES
    #define MARIADB_CLIENT_MULTI_STATEMENTS (MARIADB_FLAG_LOCAL_FILES | MARIADB_FLAG_MULTI_STATEMENTS) /* 65664 */
#endif