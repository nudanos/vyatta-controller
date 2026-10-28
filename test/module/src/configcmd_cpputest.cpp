/*
 * Copyright (c) 2026, NuDanOS.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"
#include <string.h>

extern "C" {

#include <czmq.h>
#include <json.h>
#include "controller.h"
#include "configdb.h"
#include "configcmd.h"
#include "CppUTestExt/MockSupport_c.h"

void publish_cmd(const char *topic, uint64_t seqno __attribute__((unused)),
		 const char *line, bool bin __attribute__((unused)))
{
	mock_c()->actualCall("publish_cmd")
		->withStringParameters("topic", topic)
		->withStringParameters("line", line);
}

lh_table *get_config_coll(void)
{
	return NULL;
}

}

TEST_GROUP(configcmd)
{
	void teardown(void) {
		mock().checkExpectations();
		mock().clear();
	}
};

/* A delete (ephemeral) command is published once and its text released. */
TEST(configcmd, ephemeral_published_and_released)
{
	char topic[] = "dp0s3";
	char value[] = "set";
	config_node_t node = {};
	node._topic = topic;
	node._value = value;
	command_node_t cmd = {};
	cmd._node = &node;
	cmd._ephemeral = strdup("delete");
	uint64_t seq = 0;

	mock().expectOneCall("publish_cmd")
		.withStringParameter("topic", "dp0s3")
		.withStringParameter("line", "delete");
	CHECK_EQUAL(0, publish_config_cmd(&cmd, &seq));
	CHECK_EQUAL(1, seq);
	POINTERS_EQUAL(NULL, cmd._ephemeral);
}

/* A node without a topic publishes nothing but still releases the text. */
TEST(configcmd, ephemeral_released_without_topic)
{
	char value[] = "set";
	config_node_t node = {};
	node._value = value;
	command_node_t cmd = {};
	cmd._node = &node;
	cmd._ephemeral = strdup("delete");
	uint64_t seq = 0;

	CHECK_EQUAL(0, publish_config_cmd(&cmd, &seq));
	char *left = cmd._ephemeral;
	free(left);
	POINTERS_EQUAL(NULL, left);
}

/* A set command publishes the node's value. */
TEST(configcmd, value_published)
{
	char topic[] = "dp0s3";
	char value[] = "set";
	config_node_t node = {};
	node._topic = topic;
	node._value = value;
	command_node_t cmd = {};
	cmd._node = &node;
	uint64_t seq = 4;

	mock().expectOneCall("publish_cmd")
		.withStringParameter("topic", "dp0s3")
		.withStringParameter("line", "set");
	CHECK_EQUAL(0, publish_config_cmd(&cmd, &seq));
	CHECK_EQUAL(5, node._seq);
}
