/* declare p, q and f */
#define p 17
#define q 14
#define f (1 << q)

/* convertion of numbers */
#define ntof(n) ((n) * (f))
#define xton_rz(x) ((x) / (f))
#define xton(x) ((x) > 0 ? ((x) + (f) / 2) / (f) : ((x) - (f) / 2) / (f))

/* arithmetic */
#define add_f(x, y) ((x) + (y))
#define sub_f(x, y) ((x) - (y))
#define mul_f(x, y) (((int64_t) (x)) * (y) / (f))
#define div_f(x, y) (((int64_t) (x)) * (f) / (y))
#define add_n(x, n) ((x) + (n) * (f))
#define sub_n(x, n) ((x) - (n) * (f))
#define mul_n(x, n) ((x) * (n))
#define div_n(x, n) ((x) / (n))
