/* Copyright (c) 2026 bhgv
 * EN: Exact paths and limits for session-owned Dis module loads.
 * RU: Точные пути и пределы загрузки модулей Dis в рамках сессии.
 */
#ifndef STYXRPC_LIVE_MODULE_H
#define STYXRPC_LIVE_MODULE_H

#define STYXRPC_LIVE_MODULE_ROOT ".dis-load" /* Hidden session root. */
#define STYXRPC_LIVE_MODULE_REQUEST "open" /* One request per read/write fid. */
#define STYXRPC_LIVE_MODULE_ID_BYTES 8U /* Fixed lowercase hexadecimal generation. */
#define STYXRPC_LIVE_MODULE_PATH STYXRPC_LIVE_MODULE_ROOT "/" STYXRPC_LIVE_MODULE_REQUEST

#endif
