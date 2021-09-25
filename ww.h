typedef struct {
    size_t length;
    size_t used;
    char* data;
} strbuf;

int init(strbuf*);
int append(strbuf*, char);
void wrap(strbuf*, int, int);
void printer(strbuf*, int);