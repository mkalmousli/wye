/* HTTPS downloads via WinHTTP (system proxy settings, redirects followed). */
#include "plat_sys.h"

#include <windows.h>
#include <winhttp.h>
#include <stdio.h>

static bool fetch(HINTERNET session, const char *url, FILE *out, size_t max, char *err, size_t errlen)
{
    WCHAR wurl[2048], host[256], path[2048];
    MultiByteToWideChar(CP_UTF8, 0, url, -1, wurl, 2048);
    URL_COMPONENTS u = {.dwStructSize = sizeof(u), .lpszHostName = host, .dwHostNameLength = 256,
                        .lpszUrlPath = path, .dwUrlPathLength = 2048};
    if (!WinHttpCrackUrl(wurl, 0, 0, &u) || u.nScheme != INTERNET_SCHEME_HTTPS) {
        snprintf(err, errlen, "only https URLs are allowed");
        return false;
    }
    HINTERNET conn = WinHttpConnect(session, host, u.nPort, 0);
    HINTERNET req = conn ? WinHttpOpenRequest(conn, L"GET", path, NULL, WINHTTP_NO_REFERER,
                                              WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : NULL;
    DWORD status = 0, size = sizeof(status);
    bool ok = req && WinHttpSendRequest(req, WINHTTP_NO_ADDITIONAL_HEADERS, 0, NULL, 0, 0, 0) &&
              WinHttpReceiveResponse(req, NULL) &&
              WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                  WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX) &&
              status == 200;
    size_t total = 0;
    char buf[65536];
    DWORD got = 0;
    while (ok && WinHttpReadData(req, buf, sizeof(buf), &got) && got > 0) {
        total += got;
        ok = total <= max && fwrite(buf, 1, got, out) == got;
    }
    if (!ok) {
        snprintf(err, errlen, status && status != 200 ? "server returned HTTP %lu" : "download failed (network?)",
                 status);
    }
    if (req) WinHttpCloseHandle(req);
    if (conn) WinHttpCloseHandle(conn);
    return ok;
}

bool plat_http_download(const char *url, const char *dest, size_t max_bytes, char *err, size_t errlen)
{
    HINTERNET s = WinHttpOpen(L"Wye", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME,
                              WINHTTP_NO_PROXY_BYPASS, 0);
    FILE *out = s ? fopen(dest, "wb") : NULL;
    bool ok = out && fetch(s, url, out, max_bytes, err, errlen);
    if (out) {
        ok = fclose(out) == 0 && ok;
    }
    if (!ok) {
        remove(dest);
    }
    if (s) {
        WinHttpCloseHandle(s);
    }
    return ok;
}
