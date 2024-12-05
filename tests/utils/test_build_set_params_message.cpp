#include "spa/param/param.h"
#include "spa/param/props.h"
#include "spa/pod/iter.h"
#include "spa/pod/parser.h"
#include "spa/utils/type.h"
#include <microtest/microtest.h>
#include <sys/types.h>
#include <utils/pod_message_builder.h>

TEST(BuildSetParamsMessageShouldReturnAValidObject) {
  u_int8_t buffer[1024];
  auto pod = utils::PodMessageBuilder::build_set_params_message(
      buffer, sizeof(buffer), "ParamName", "5667.77");
  ASSERT_NOTNULL(pod);
  ASSERT_TRUE(spa_pod_is_object(pod));
}

TEST(BuildSetParamsMessageShouldReturnAnObjectWithAParamProperty) {
  u_int8_t buffer[1024];
  auto pod = utils::PodMessageBuilder::build_set_params_message(
      buffer, sizeof(buffer), "ParamName", "5667.77");

  struct spa_pod_prop *property;
  struct spa_pod_object *object = (struct spa_pod_object *)pod;
  unsigned int iterations = 0;
  SPA_POD_OBJECT_FOREACH(object, property) {
    iterations++;
    ASSERT_EQ(property->key, SPA_PROP_params);
    ASSERT_TRUE(spa_pod_is_struct(&property->value));

    struct spa_pod_parser parser;
    spa_pod_parser_pod(&parser, &property->value);

    struct spa_pod_frame struct_frame;
    spa_pod_parser_push_struct(&parser, &struct_frame);

    const char *param_name;
    spa_pod_parser_get_string(&parser, &param_name);
    ASSERT_STREQ(param_name, "ParamName");

    const char *param_value;
    spa_pod_parser_get_string(&parser, &param_value);
    ASSERT_STREQ(param_value, "5667.77");
  }
  ASSERT_EQ(iterations, 1);
}

TEST_MAIN();
