#include <Arduino.h>
#include <hashmap.h>

void setup()
{
    any_key_t** keys;
    any_value_t* value;
    size_t keys_len, i;
    hashmap_ptr hmap;

    Serial.begin(9600);

    /* load some data */
    hmap = hashmap_new(8, NULL, NULL);
    Serial.print("ADDITION..\n");
    hashmap_put(hmap, "L01-key", "L01-value");
    hashmap_put(hmap, "L02-key", "L02-value");
    hashmap_put(hmap, "L03-key", "L03-value");
    hashmap_put(hmap, "S01-key", "S01-value");
    hashmap_put(hmap, "S02-key", "S02-value");
    hashmap_put(hmap, "S03-key", "S03-value");
    hashmap_put(hmap, "C01-key", "C01-value");
    hashmap_put(hmap, "C02-key", "C02-value");
    hashmap_put(hmap, "C03-key", "C03-value");

    /* print keys */
    Serial.print("FIRST PRINT..\n");
    if (HASHMAP_OK == hashmap_keys(hmap, &keys, &keys_len)) {
        for (i = 0; i < keys_len; ++i) {
            if (HASHMAP_OK == hashmap_get(hmap, keys[i], &value)) {
                Serial.print("Bucket: ");
                Serial.print((char*) keys[i]);
                Serial.print(" --> ");
                Serial.print((char*) value);
                Serial.print("\n");
            } else {
                Serial.print("Error on key: ");
                Serial.print((char*) keys[i]);
                Serial.print("\n");
            }
        }
    } else {
        Serial.print("Unable to retrieve keys.\n");
    }

    /* remove items */
    Serial.print("REMOVAL..\n");
    hashmap_remove(hmap, "L03-key", NULL);
    hashmap_remove(hmap, "S03-key", NULL);
    hashmap_remove(hmap, "C03-key", NULL);

    /* print keys */
    Serial.print("SECOND PRINT..\n");
    if (HASHMAP_OK == hashmap_keys(hmap, &keys, &keys_len)) {
        for (i = 0; i < keys_len; ++i) {
            if (HASHMAP_OK == hashmap_get(hmap, keys[i], &value)) {
                Serial.print("Bucket: ");
                Serial.print((char*) keys[i]);
                Serial.print(" --> ");
                Serial.print((char*) value);
                Serial.print("\n");
            } else {
                Serial.print("Error on key: ");
                Serial.print((char*) keys[i]);
                Serial.print("\n");
            }
        }
    } else {
        Serial.print("Unable to retrieve keys.\n");
    }

    /* apply iterative function */
    Serial.print("APPLY FUNCTION..\n");
    hashmap_iterate_check_ok_status(hmap,
                   (hashmap_fun) my_serial_print, (void*) "Value: ");
}


void loop()
{
    // nothing to do
}

/**
 * my_serial_print
 * prints on serial a string ``str'' followed by ``value'' and
 * '\n'-terminates the message
 */
int my_serial_print(const char* str, const char* value) {
  Serial.print(str);
  Serial.print((char*)value);
  Serial.print("\n");
  return HASHMAP_OK;
}
