# Session-only recent paths; UI labels never carry file authority.
Iderecent: module
{
    PATH: con "iderecent.dis"; # Matching module resource.
    remember: fn(paths: array of string, path: string): array of string; # Fresh four-entry most-recent-first copy.
    labels: fn(paths: array of string): string; # Four newline-delimited sanitized short display labels.
};
