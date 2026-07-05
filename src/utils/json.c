#include "utils/json.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Value representation
// ---------------------------------------------------------------------------

typedef struct JsonMember {
    char *key;
    JsonValue *value;
} JsonMember;

struct JsonValue {
    JsonType type;
    union {
        bool boolean;
        struct { double value; bool isInteger; } number;
        char *string;
        struct {
            JsonValue **items;
            int count;
            int capacity;
        } array;
        struct {
            JsonMember *members;
            int count;
            int capacity;
        } object;
    } as;
};

static JsonValue *allocValue(JsonType type) {
    JsonValue *value = (JsonValue *)calloc(1, sizeof(JsonValue));
    if (value) value->type = type;
    return value;
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

JsonValue *jsonNewObject(void) { return allocValue(JSON_OBJECT); }
JsonValue *jsonNewArray(void)  { return allocValue(JSON_ARRAY); }
JsonValue *jsonNewNull(void)   { return allocValue(JSON_NULL); }

JsonValue *jsonNewBool(bool value) {
    JsonValue *v = allocValue(JSON_BOOL);
    if (v) v->as.boolean = value;
    return v;
}

JsonValue *jsonNewInt(long long number) {
    JsonValue *v = allocValue(JSON_NUMBER);
    if (v) { v->as.number.value = (double)number; v->as.number.isInteger = true; }
    return v;
}

JsonValue *jsonNewDouble(double number) {
    JsonValue *v = allocValue(JSON_NUMBER);
    if (v) { v->as.number.value = number; v->as.number.isInteger = false; }
    return v;
}

JsonValue *jsonNewString(const char *string) {
    JsonValue *v = allocValue(JSON_STRING);
    if (!v) return NULL;
    v->as.string = _strdup(string ? string : "");
    if (!v->as.string) { free(v); return NULL; }
    return v;
}

// ---------------------------------------------------------------------------
// Destruction
// ---------------------------------------------------------------------------

void jsonFree(JsonValue *value) {
    if (!value) return;
    switch (value->type) {
        case JSON_STRING:
            free(value->as.string);
            break;
        case JSON_ARRAY:
            for (int i = 0; i < value->as.array.count; i++) {
                jsonFree(value->as.array.items[i]);
            }
            free(value->as.array.items);
            break;
        case JSON_OBJECT:
            for (int i = 0; i < value->as.object.count; i++) {
                free(value->as.object.members[i].key);
                jsonFree(value->as.object.members[i].value);
            }
            free(value->as.object.members);
            break;
        default:
            break;
    }
    free(value);
}

// ---------------------------------------------------------------------------
// Mutation
// ---------------------------------------------------------------------------

void jsonArrayAppend(JsonValue *array, JsonValue *child) {
    if (!array || array->type != JSON_ARRAY || !child) return;
    if (array->as.array.count == array->as.array.capacity) {
        int cap = array->as.array.capacity ? array->as.array.capacity * 2 : 4;
        JsonValue **items = (JsonValue **)realloc(array->as.array.items, cap * sizeof(JsonValue *));
        if (!items) return;
        array->as.array.items = items;
        array->as.array.capacity = cap;
    }
    array->as.array.items[array->as.array.count++] = child;
}

void jsonObjectSet(JsonValue *object, const char *key, JsonValue *child) {
    if (!object || object->type != JSON_OBJECT || !key || !child) return;
    for (int i = 0; i < object->as.object.count; i++) {
        if (strcmp(object->as.object.members[i].key, key) == 0) {
            jsonFree(object->as.object.members[i].value);
            object->as.object.members[i].value = child;
            return;
        }
    }
    if (object->as.object.count == object->as.object.capacity) {
        int cap = object->as.object.capacity ? object->as.object.capacity * 2 : 4;
        JsonMember *members = (JsonMember *)realloc(object->as.object.members, cap * sizeof(JsonMember));
        if (!members) return;
        object->as.object.members = members;
        object->as.object.capacity = cap;
    }
    object->as.object.members[object->as.object.count].key = _strdup(key);
    object->as.object.members[object->as.object.count].value = child;
    object->as.object.count++;
}

void jsonObjectSetString(JsonValue *object, const char *key, const char *string) {
    jsonObjectSet(object, key, jsonNewString(string));
}
void jsonObjectSetInt(JsonValue *object, const char *key, long long number) {
    jsonObjectSet(object, key, jsonNewInt(number));
}
void jsonObjectSetDouble(JsonValue *object, const char *key, double number) {
    jsonObjectSet(object, key, jsonNewDouble(number));
}
void jsonObjectSetBool(JsonValue *object, const char *key, bool value) {
    jsonObjectSet(object, key, jsonNewBool(value));
}
void jsonObjectSetNull(JsonValue *object, const char *key) {
    jsonObjectSet(object, key, jsonNewNull());
}

// ---------------------------------------------------------------------------
// Inspection & access
// ---------------------------------------------------------------------------

JsonType jsonTypeOf(const JsonValue *value) { return value ? value->type : JSON_NULL; }
bool jsonIsNull(const JsonValue *value) { return !value || value->type == JSON_NULL; }

bool jsonGetBool(const JsonValue *value, bool fallback) {
    return (value && value->type == JSON_BOOL) ? value->as.boolean : fallback;
}
double jsonGetNumber(const JsonValue *value, double fallback) {
    return (value && value->type == JSON_NUMBER) ? value->as.number.value : fallback;
}
int jsonGetInt(const JsonValue *value, int fallback) {
    return (value && value->type == JSON_NUMBER) ? (int)value->as.number.value : fallback;
}
const char *jsonGetString(const JsonValue *value, const char *fallback) {
    return (value && value->type == JSON_STRING) ? value->as.string : fallback;
}

JsonValue *jsonObjectGet(const JsonValue *object, const char *key) {
    if (!object || object->type != JSON_OBJECT || !key) return NULL;
    for (int i = 0; i < object->as.object.count; i++) {
        if (strcmp(object->as.object.members[i].key, key) == 0) {
            return object->as.object.members[i].value;
        }
    }
    return NULL;
}
bool jsonObjectHas(const JsonValue *object, const char *key) {
    return jsonObjectGet(object, key) != NULL;
}
int jsonObjectCount(const JsonValue *object) {
    return (object && object->type == JSON_OBJECT) ? object->as.object.count : 0;
}
const char *jsonObjectKeyAt(const JsonValue *object, int index) {
    if (!object || object->type != JSON_OBJECT) return NULL;
    if (index < 0 || index >= object->as.object.count) return NULL;
    return object->as.object.members[index].key;
}
JsonValue *jsonObjectValueAt(const JsonValue *object, int index) {
    if (!object || object->type != JSON_OBJECT) return NULL;
    if (index < 0 || index >= object->as.object.count) return NULL;
    return object->as.object.members[index].value;
}

bool jsonObjectGetBool(const JsonValue *object, const char *key, bool fallback) {
    return jsonGetBool(jsonObjectGet(object, key), fallback);
}
double jsonObjectGetNumber(const JsonValue *object, const char *key, double fallback) {
    return jsonGetNumber(jsonObjectGet(object, key), fallback);
}
int jsonObjectGetInt(const JsonValue *object, const char *key, int fallback) {
    return jsonGetInt(jsonObjectGet(object, key), fallback);
}
const char *jsonObjectGetString(const JsonValue *object, const char *key, const char *fallback) {
    return jsonGetString(jsonObjectGet(object, key), fallback);
}

int jsonArrayCount(const JsonValue *array) {
    return (array && array->type == JSON_ARRAY) ? array->as.array.count : 0;
}
JsonValue *jsonArrayAt(const JsonValue *array, int index) {
    if (!array || array->type != JSON_ARRAY) return NULL;
    if (index < 0 || index >= array->as.array.count) return NULL;
    return array->as.array.items[index];
}

// ---------------------------------------------------------------------------
// Parsing (recursive descent)
// ---------------------------------------------------------------------------

typedef struct {
    const char *p;   // current position
    bool error;
} Parser;

static JsonValue *parseValue(Parser *parser);

static void skipWhitespace(Parser *parser) {
    while (*parser->p == ' ' || *parser->p == '\t' ||
           *parser->p == '\n' || *parser->p == '\r') {
        parser->p++;
    }
}

static char *parseStringRaw(Parser *parser) {
    parser->p++;
    size_t cap = 16, len = 0;
    char *out = (char *)malloc(cap);
    if (!out) { parser->error = true; return NULL; }

    while (*parser->p && *parser->p != '"') {
        char c = *parser->p;
        if (c == '\\') {
            parser->p++;
            char esc = *parser->p;
            switch (esc) {
                case '"':  c = '"';  break;
                case '\\': c = '\\'; break;
                case '/':  c = '/';  break;
                case 'b':  c = '\b'; break;
                case 'f':  c = '\f'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                case 't':  c = '\t'; break;
                case 'u': {
                    // Decode a \uXXXX escape into UTF-8 (BMP only).
                    unsigned int code = 0;
                    for (int i = 0; i < 4; i++) {
                        char h = parser->p[1 + i];
                        code <<= 4;
                        if (h >= '0' && h <= '9') code |= (unsigned)(h - '0');
                        else if (h >= 'a' && h <= 'f') code |= (unsigned)(h - 'a' + 10);
                        else if (h >= 'A' && h <= 'F') code |= (unsigned)(h - 'A' + 10);
                        else { parser->error = true; free(out); return NULL; }
                    }
                    parser->p += 4;
                    char utf8[4];
                    int n = 0;
                    if (code < 0x80) {
                        utf8[n++] = (char)code;
                    } else if (code < 0x800) {
                        utf8[n++] = (char)(0xC0 | (code >> 6));
                        utf8[n++] = (char)(0x80 | (code & 0x3F));
                    } else {
                        utf8[n++] = (char)(0xE0 | (code >> 12));
                        utf8[n++] = (char)(0x80 | ((code >> 6) & 0x3F));
                        utf8[n++] = (char)(0x80 | (code & 0x3F));
                    }
                    for (int i = 0; i < n; i++) {
                        if (len + 1 >= cap) {
                            cap *= 2;
                            char *grown = (char *)realloc(out, cap);
                            if (!grown) { parser->error = true; free(out); return NULL; }
                            out = grown;
                        }
                        out[len++] = utf8[i];
                    }
                    parser->p++; // consume last hex digit; loop increment handles the rest
                    continue;
                }
                default:
                    parser->error = true; free(out); return NULL;
            }
        }
        if (len + 1 >= cap) {
            cap *= 2;
            char *grown = (char *)realloc(out, cap);
            if (!grown) { parser->error = true; free(out); return NULL; }
            out = grown;
        }
        out[len++] = c;
        parser->p++;
    }

    if (*parser->p != '"') { parser->error = true; free(out); return NULL; }
    parser->p++; // consume closing quote
    out[len] = '\0';
    return out;
}

static JsonValue *parseString(Parser *parser) {
    char *s = parseStringRaw(parser);
    if (!s) return NULL;
    JsonValue *v = allocValue(JSON_STRING);
    if (!v) { free(s); parser->error = true; return NULL; }
    v->as.string = s;
    return v;
}

static JsonValue *parseNumber(Parser *parser) {
    const char *start = parser->p;
    bool isInteger = true;
    if (*parser->p == '-') parser->p++;
    while (*parser->p >= '0' && *parser->p <= '9') parser->p++;
    if (*parser->p == '.') { isInteger = false; parser->p++; while (*parser->p >= '0' && *parser->p <= '9') parser->p++; }
    if (*parser->p == 'e' || *parser->p == 'E') {
        isInteger = false;
        parser->p++;
        if (*parser->p == '+' || *parser->p == '-') parser->p++;
        while (*parser->p >= '0' && *parser->p <= '9') parser->p++;
    }
    JsonValue *v = allocValue(JSON_NUMBER);
    if (!v) { parser->error = true; return NULL; }
    v->as.number.value = strtod(start, NULL);
    v->as.number.isInteger = isInteger;
    return v;
}

static JsonValue *parseLiteral(Parser *parser) {
    if (strncmp(parser->p, "true", 4) == 0)  { parser->p += 4; return jsonNewBool(true); }
    if (strncmp(parser->p, "false", 5) == 0) { parser->p += 5; return jsonNewBool(false); }
    if (strncmp(parser->p, "null", 4) == 0)  { parser->p += 4; return jsonNewNull(); }
    parser->error = true;
    return NULL;
}

static JsonValue *parseArray(Parser *parser) {
    parser->p++;
    JsonValue *arr = jsonNewArray();
    if (!arr) { parser->error = true; return NULL; }
    skipWhitespace(parser);
    if (*parser->p == ']') { parser->p++; return arr; }
    while (true) {
        skipWhitespace(parser);
        JsonValue *item = parseValue(parser);
        if (parser->error) { jsonFree(arr); return NULL; }
        jsonArrayAppend(arr, item);
        skipWhitespace(parser);
        if (*parser->p == ',') { parser->p++; continue; }
        if (*parser->p == ']') { parser->p++; break; }
        parser->error = true; jsonFree(arr); return NULL;
    }
    return arr;
}

static JsonValue *parseObject(Parser *parser) {
    parser->p++;
    JsonValue *obj = jsonNewObject();
    if (!obj) { parser->error = true; return NULL; }
    skipWhitespace(parser);
    if (*parser->p == '}') { parser->p++; return obj; }
    while (true) {
        skipWhitespace(parser);
        if (*parser->p != '"') { parser->error = true; jsonFree(obj); return NULL; }
        char *key = parseStringRaw(parser);
        if (!key) { jsonFree(obj); return NULL; }
        skipWhitespace(parser);
        if (*parser->p != ':') { parser->error = true; free(key); jsonFree(obj); return NULL; }
        parser->p++;
        skipWhitespace(parser);
        JsonValue *val = parseValue(parser);
        if (parser->error) { free(key); jsonFree(obj); return NULL; }
        jsonObjectSet(obj, key, val);
        free(key);
        skipWhitespace(parser);
        if (*parser->p == ',') { parser->p++; continue; }
        if (*parser->p == '}') { parser->p++; break; }
        parser->error = true; jsonFree(obj); return NULL;
    }
    return obj;
}

static JsonValue *parseValue(Parser *parser) {
    skipWhitespace(parser);
    char c = *parser->p;
    switch (c) {
        case '{': return parseObject(parser);
        case '[': return parseArray(parser);
        case '"': return parseString(parser);
        case 't': case 'f': case 'n': return parseLiteral(parser);
        default:
            if (c == '-' || (c >= '0' && c <= '9')) return parseNumber(parser);
            parser->error = true;
            return NULL;
    }
}

JsonValue *jsonParse(const char *text) {
    if (!text) return NULL;
    Parser parser = { text, false };
    JsonValue *value = parseValue(&parser);
    if (parser.error) { jsonFree(value); return NULL; }
    skipWhitespace(&parser);
    if (*parser.p != '\0') { jsonFree(value); return NULL; } // trailing garbage
    return value;
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

typedef struct {
    char *buf;
    size_t len;
    size_t cap;
    bool error;
    bool pretty;      // emit newlines + indentation
    int indentWidth;  // spaces per nesting level (pretty only)
} Writer;

static void writerEnsure(Writer *writer, size_t extra) {
    if (writer->error) return;
    if (writer->len + extra + 1 > writer->cap) {
        size_t cap = writer->cap ? writer->cap : 64;
        while (writer->len + extra + 1 > cap) cap *= 2;
        char *grown = (char *)realloc(writer->buf, cap);
        if (!grown) { writer->error = true; return; }
        writer->buf = grown;
        writer->cap = cap;
    }
}

static void writerPutChar(Writer *writer, char c) {
    writerEnsure(writer, 1);
    if (writer->error) return;
    writer->buf[writer->len++] = c;
}

static void writerPutStr(Writer *writer, const char *s) {
    size_t n = strlen(s);
    writerEnsure(writer, n);
    if (writer->error) return;
    memcpy(writer->buf + writer->len, s, n);
    writer->len += n;
}

static void writerPutEscaped(Writer *writer, const char *s) {
    writerPutChar(writer, '"');
    for (const char *c = s; *c; c++) {
        unsigned char ch = (unsigned char)*c;
        switch (ch) {
            case '"':  writerPutStr(writer, "\\\""); break;
            case '\\': writerPutStr(writer, "\\\\"); break;
            case '\b': writerPutStr(writer, "\\b"); break;
            case '\f': writerPutStr(writer, "\\f"); break;
            case '\n': writerPutStr(writer, "\\n"); break;
            case '\r': writerPutStr(writer, "\\r"); break;
            case '\t': writerPutStr(writer, "\\t"); break;
            default:
                if (ch < 0x20) {
                    char esc[8];
                    snprintf(esc, sizeof(esc), "\\u%04x", ch);
                    writerPutStr(writer, esc);
                } else {
                    writerPutChar(writer, (char)ch);
                }
        }
    }
    writerPutChar(writer, '"');
}

static void writerNewlineIndent(Writer *writer, int depth) {
    if (!writer->pretty) return;
    writerPutChar(writer, '\n');
    int spaces = depth * writer->indentWidth;
    for (int i = 0; i < spaces; i++) writerPutChar(writer, ' ');
}

static void writeValue(Writer *writer, const JsonValue *value, int depth) {
    if (writer->error) return;
    if (!value) { writerPutStr(writer, "null"); return; }
    switch (value->type) {
        case JSON_NULL:
            writerPutStr(writer, "null");
            break;
        case JSON_BOOL:
            writerPutStr(writer, value->as.boolean ? "true" : "false");
            break;
        case JSON_NUMBER: {
            char num[32];
            double d = value->as.number.value;
            if (value->as.number.isInteger) {
                snprintf(num, sizeof(num), "%lld", (long long)d);
            } else if (d != d || d > 1e308 || d < -1e308) {
                // NaN or +/-inf are not valid JSON numbers; emit null.
                snprintf(num, sizeof(num), "null");
            } else {
                snprintf(num, sizeof(num), "%.10g", d);
            }
            writerPutStr(writer, num);
            break;
        }
        case JSON_STRING:
            writerPutEscaped(writer, value->as.string);
            break;
        case JSON_ARRAY:
            if (value->as.array.count == 0) { writerPutStr(writer, "[]"); break; }
            writerPutChar(writer, '[');
            for (int i = 0; i < value->as.array.count; i++) {
                if (i) writerPutChar(writer, ',');
                writerNewlineIndent(writer, depth + 1);
                writeValue(writer, value->as.array.items[i], depth + 1);
            }
            writerNewlineIndent(writer, depth);
            writerPutChar(writer, ']');
            break;
        case JSON_OBJECT:
            if (value->as.object.count == 0) { writerPutStr(writer, "{}"); break; }
            writerPutChar(writer, '{');
            for (int i = 0; i < value->as.object.count; i++) {
                if (i) writerPutChar(writer, ',');
                writerNewlineIndent(writer, depth + 1);
                writerPutEscaped(writer, value->as.object.members[i].key);
                writerPutChar(writer, ':');
                if (writer->pretty) writerPutChar(writer, ' ');
                writeValue(writer, value->as.object.members[i].value, depth + 1);
            }
            writerNewlineIndent(writer, depth);
            writerPutChar(writer, '}');
            break;
    }
}

static char *serialize(const JsonValue *value, bool pretty, int indentWidth) {
    Writer writer = { NULL, 0, 0, false, pretty, indentWidth };
    writeValue(&writer, value, 0);
    if (writer.error) { free(writer.buf); return NULL; }
    writerEnsure(&writer, 0);
    if (writer.error) { free(writer.buf); return NULL; }
    writer.buf[writer.len] = '\0';
    return writer.buf;
}

char *jsonSerialize(const JsonValue *value) {
    return serialize(value, false, 0);
}

char *jsonSerializePretty(const JsonValue *value, int indentWidth) {
    if (indentWidth < 0) indentWidth = 0;
    return serialize(value, true, indentWidth);
}
