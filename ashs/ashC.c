#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#define A 256
#define B 4096

typedef struct X { pid_t p; struct X *n; } X; static X *Y = NULL;

static void Z(pid_t p) { X *x = malloc(sizeof(*x)); if (!x) return; x->p = p; x->n = Y; Y = x; }
static void W(pid_t p) { X **x = &Y; while (*x) { if ((*x)->p == p) { X *t = *x; *x = t->n; free(t); return; } x = &(*x)->n; } }

static void V(int s) { (void)s; int st; pid_t p; while ((p = waitpid(-1, &st, WNOHANG)) > 0) {
    if (WIFEXITED(st)) printf("\n[ashS] %d exited %d\n", p, WEXITSTATUS(st));
    else if (WIFSIGNALED(st)) printf("\n[ashS] %d killed by %d\n", p, WTERMSIG(st));
    W(p); } fflush(stdout); }

static void U(int s) { (void)s; write(1, "\n", 1); }

static char *T(char *s) { while (*s && (*s == ' ' || *s == '\t' || *s == '\n')) s++; if (*s == 0) return s;
    char *e = s + strlen(s) - 1; while (e > s && (*e == ' ' || *e == '\t' || *e == '\n')) { *e = 0; e--; } return s; }

static int S(char *l, char **t) { int n = 0; char *p = l; while (*p) {
    while (*p == ' ' || *p == '\t' || *p == '\n') p++; if (!*p) break;
    if (*p == '|' || *p == '<' || *p == '&') { char tmp[2] = {*p, 0}; t[n++] = strdup(tmp); p++; }
    else if (*p == '>') { if (*(p+1) == '>') { t[n++] = strdup(">>"); p += 2; } else { t[n++] = strdup(">"); p++; } }
    else if (*p == '\'' || *p == '"') { char q = *p++; char *s = p; while (*p && *p != q) p++; size_t l = p - s;
        char *tok = malloc(l + 1); memcpy(tok, s, l); tok[l] = 0; t[n++] = tok; if (*p == q) p++; }
    else { char *s = p; while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '|' && *p != '<' && *p != '>' && *p != '&') p++;
        size_t l = p - s; char *tok = malloc(l + 1); memcpy(tok, s, l); tok[l] = 0; t[n++] = tok; } } t[n] = NULL; return n; }

static void R(char **t, int n) { for (int i = 0; i < n; ++i) free(t[i]); }

struct C { char *v[A]; char *i; char *o; int a; int b; };

static int Q(char **t, int n, struct C *c, int *bg) {
    int ci = 0, ai = 0; memset(&c[0], 0, sizeof(struct C) * (n+1)); c[0].i = c[0].o = NULL; c[0].a = 0; *bg = 0;
    for (int i = 0; i < n; ++i) { char *x = t[i];
        if (strcmp(x, "|") == 0) { c[ci].v[ai] = NULL; ci++; ai = 0; c[ci].i = c[ci].o = NULL; c[ci].a = 0; }
        else if (strcmp(x, "<") == 0) { if (i+1 < n) { c[ci].i = strdup(t[++i]); } }
        else if (strcmp(x, ">") == 0) { if (i+1 < n) { c[ci].o = strdup(t[++i]); c[ci].a = 0; } }
        else if (strcmp(x, ">>") == 0) { if (i+1 < n) { c[ci].o = strdup(t[++i]); c[ci].a = 1; } }
        else if (strcmp(x, "&") == 0) { *bg = 1; c[ci].b = 1; }
        else { c[ci].v[ai++] = strdup(x); } } c[ci].v[ai] = NULL; return ci + 1; }

static int P(char *c) { if (!c) return 0; return (strcmp(c, "cd") == 0 || strcmp(c, "exit") == 0 || strcmp(c, "help") == 0); }

static void O(struct C *c, int n, int bg) {
    int i; int pf = -1; int pfd[2]; pid_t pid; int inf = -1, outf = -1;
    if (n == 1 && c[0].v[0] && P(c[0].v[0]) && !bg) {
        if (strcmp(c[0].v[0], "cd") == 0) { char *d = c[0].v[1] ? c[0].v[1] : getenv("HOME"); if (chdir(d) < 0) perror("cd"); }
        else if (strcmp(c[0].v[0], "exit") == 0) { int co = c[0].v[1] ? atoi(c[0].v[1]) : 0; exit(co); }
        else if (strcmp(c[0].v[0], "help") == 0) {
            printf("ashS - simple shell\nBuiltins: cd, exit, help\nSupports: < > >> | &\n"); } return; }
    for (i = 0; i < n; ++i) { if (i < n - 1) { if (pipe(pfd) < 0) { perror("pipe"); return; } }
        pid = fork(); if (pid < 0) { perror("fork"); return; } if (pid == 0) {
            signal(SIGINT, SIG_DFL); if (pf != -1) { dup2(pf, 0); close(pf); } else if (c[i].i) {
                inf = open(c[i].i, O_RDONLY); if (inf < 0) { perror(c[i].i); exit(1); } dup2(inf, 0); close(inf); }
            if (i < n - 1) { close(pfd[0]); dup2(pfd[1], 1); close(pfd[1]); } else if (c[i].o) {
                int fl = O_WRONLY | O_CREAT | (c[i].a ? O_APPEND : O_TRUNC);
                outf = open(c[i].o, fl, 0666); if (outf < 0) { perror(c[i].o); exit(1); } dup2(outf, 1); close(outf); }
            if (P(c[i].v[0])) { if (strcmp(c[i].v[0], "cd") == 0) {
                    char *d = c[i].v[1] ? c[i].v[1] : getenv("HOME"); if (chdir(d) < 0) perror("cd"); exit(0); }
                else if (strcmp(c[i].v[0], "exit") == 0) exit(0);
                else if (strcmp(c[i].v[0], "help") == 0) {
                    printf("ashS - simple shell\nBuiltins: cd, exit, help\nSupports: < > >> | &\n"); exit(0); } }
            execvp(c[i].v[0], c[i].v); fprintf(stderr, "ashS: %s: %s\n", c[i].v[0], strerror(errno)); exit(127); }
        else { if (pf != -1) close(pf); if (i < n - 1) { close(pfd[1]); pf = pfd[0]; }
            if (!bg) { int st; waitpid(pid, &st, 0); } else { printf("[ashS] bg %d\n", pid); Z(pid); } } }
    if (pf != -1) close(pf); }

static void N(struct C *c, int n) {
    for (int i = 0; i < n; ++i) { for (int j = 0; c[i].v[j]; ++j) free(c[i].v[j]);
        if (c[i].i) free(c[i].i); if (c[i].o) free(c[i].o); } }

int main(void) {
    char l[B]; char *t[A]; struct sigaction sc = {0}, si = {0};
    sc.sa_handler = V; sigemptyset(&sc.sa_mask); sc.sa_flags = SA_RESTART | SA_NOCLDSTOP; sigaction(SIGCHLD, &sc, NULL);
    si.sa_handler = U; sigemptyset(&si.sa_mask); si.sa_flags = SA_RESTART; sigaction(SIGINT, &si, NULL);
    while (1) { char w[1024]; if (getcwd(w, sizeof(w))) printf("ashS:%s$ ", w); else printf("ashS:$ "); fflush(stdout);
        if (!fgets(l, sizeof(l), stdin)) { putchar('\n'); break; } char *ln = T(l); if (ln[0] == 0) continue;
        int nt = S(ln, t); if (nt == 0) continue; struct C c[A]; int bg = 0; int nc = Q(t, nt, c, &bg);
        O(c, nc, bg); N(c, nc); R(t, nt); } return 0; }
