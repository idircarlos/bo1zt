#ifndef UTILS_JSON_H_
#define UTILS_JSON_H_

#include <stdbool.h>

typedef enum {
    JSON_NULL,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT,
} JsonType;

typedef struct JsonValue JsonValue;

JsonValue *jsonParse(const char *text);
void jsonFree(JsonValue *value);

JsonType jsonTypeOf(const JsonValue *value);
bool jsonIsNull(const JsonValue *value);

bool jsonGetBool(const JsonValue *value, bool fallback);
double jsonGetNumber(const JsonValue *value, double fallback);
int jsonGetInt(const JsonValue *value, int fallback);
const char *jsonGetString(const JsonValue *value, const char *fallback);

JsonValue *jsonObjectGet(const JsonValue *object, const char *key); // NULL if absent
bool jsonObjectHas(const JsonValue *object, const char *key);
int jsonObjectCount(const JsonValue *object);
const char *jsonObjectKeyAt(const JsonValue *object, int index);
JsonValue *jsonObjectValueAt(const JsonValue *object, int index);

bool jsonObjectGetBool(const JsonValue *object, const char *key, bool fallback);
double jsonObjectGetNumber(const JsonValue *object, const char *key, double fallback);
int jsonObjectGetInt(const JsonValue *object, const char *key, int fallback);
const char *jsonObjectGetString(const JsonValue *object, const char *key, const char *fallback);

int jsonArrayCount(const JsonValue *array);
JsonValue *jsonArrayAt(const JsonValue *array, int index);

JsonValue *jsonNewObject(void);
JsonValue *jsonNewArray(void);
JsonValue *jsonNewString(const char *string);
JsonValue *jsonNewInt(long long number);
JsonValue *jsonNewDouble(double number);
JsonValue *jsonNewBool(bool value);
JsonValue *jsonNewNull(void);

void jsonObjectSet(JsonValue *object, const char *key, JsonValue *child);
void jsonObjectSetString(JsonValue *object, const char *key, const char *string);
void jsonObjectSetInt(JsonValue *object, const char *key, long long number);
void jsonObjectSetDouble(JsonValue *object, const char *key, double number);
void jsonObjectSetBool(JsonValue *object, const char *key, bool value);
void jsonObjectSetNull(JsonValue *object, const char *key);
void jsonArrayAppend(JsonValue *array, JsonValue *child);

char *jsonSerialize(const JsonValue *value);

// Serialize with indentation. indentWidth is spaces per nesting level (e.g. 2 or 4). Caller frees. NULL on error.
char *jsonSerializePretty(const JsonValue *value, int indentWidth);

#endif // UTILS_JSON_H_
