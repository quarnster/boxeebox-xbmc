#include <platform_config.h>

config_result_t config_get_int(
            config_ref_t    base_ref,
            const char *    name,
            int *           val ) {}

config_result_t config_node_find(
            config_ref_t    base_ref,
            const char *    name,
            config_ref_t *  node_ref ) {}
