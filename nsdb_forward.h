#ifndef NSDB_FORWARD_H
#define NSDB_FORWARD_H

/*
 * Forward declarations for the nsdb database under test and its supporting
 * helpers. The actual definitions live elsewhere; this header just lets the
 * swarm test compile against them.
 */

typedef struct nsdb  nsdb_t;
typedef struct error error;
typedef struct type  type;
typedef int          sb_size;
typedef unsigned int b_size;

/* --- Type / random helpers (provided by the test environment) ---------- */
int          type_byte_size (const struct type *t);
struct type *random_type (void);
char        *type_str (struct type *t);
char        *random_name (int min, int max);
void         type_free (struct type *t);
int          nsdb_validate (nsdb_t *db);

/* --- Lifecycle --------------------------------------------------------- */
nsdb_t *nsdb_open (const char *path);
int     nsdb_cleanup (const char *path);
nsdb_t *nsdb_new_context (nsdb_t *ns);
int     nsdb_close (nsdb_t *ns);
int     nsdb_crash (nsdb_t *ns);

/* --- Simple api -------------------------------------------------------- */
const char *nsdb_strerror (nsdb_t *ns);
int         nsdb_perror (nsdb_t *ns, const char *prefix);

/* --- Transaction support ---------------------------------------------- */
int nsdb_begin (nsdb_t *ns);
int nsdb_commit (nsdb_t *ns);
int nsdb_rollback (nsdb_t *ns);

/* --- Variable API ----------------------------------------------------- */
int nsdb_create (nsdb_t *ns, const char *name, const char *type);
int nsdb_delete (nsdb_t *ns, const char *name);

/* --- Primary API ------------------------------------------------------ */
sb_size nsdb_len (nsdb_t *ns, const char *vname);

/* In array notation [a:b:c] */
#define STOP_PRESENT  (1 << 0) /* [:c] */
#define STEP_PRESENT  (1 << 1) /* [:b:] */
#define START_PRESENT (1 << 2) /* [a:] */
#define COLON_PRESENT (1 << 3) /* [:]  */

sb_size nsdb_insert (nsdb_t *ns, const char *name, const void *src, sb_size ofst, b_size slen);

sb_size nsdb_write (
    nsdb_t     *ns,
    const char *name,
    const void *src,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags);

sb_size nsdb_read (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags);

sb_size nsdb_remove (
    nsdb_t     *ns,
    const char *name,
    void       *dest,
    sb_size     start,
    sb_size     step,
    sb_size     stop,
    int         flags);

#endif /* NSDB_FORWARD_H */
