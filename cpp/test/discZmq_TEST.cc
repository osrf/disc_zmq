#include <limits.h>
#include "../discZmq.hh"
#include "gtest/gtest.h"

bool callbackExecuted;

//  ---------------------------------------------------------------------
/// \brief Function is called everytime a topic update is received.
void cb(const std::string &_topic, const std::string &_data)
{
  assert(_topic != "");
  EXPECT_EQ(_data, "someData");
  callbackExecuted = true;
}

//  ---------------------------------------------------------------------
TEST(DiscZmqTest, PubWithoutAdvertise)
{
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	// Subscribe to topic1
	Node node(master, verbose);

	// Publish some data on topic1 without advertising it first
	EXPECT_NE(node.Publish(topic1, data), 0);
}

//  ---------------------------------------------------------------------
TEST(DiscZmqTest, PubSub)
{
	callbackExecuted = false;
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	// Subscribe to topic1
	Node node(master, verbose);
	EXPECT_EQ(node.Subscribe(topic1, cb), 0);
	node.SpinOnce();

	// Advertise and publish some data on topic1
	EXPECT_EQ(node.Advertise(topic1), 0);
	EXPECT_EQ(node.Publish(topic1, data), 0);
	s_sleep(100);
	node.SpinOnce();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;

	// Publish a second message on topic1
	EXPECT_EQ(node.Publish(topic1, data), 0);
	s_sleep(100);
	node.SpinOnce();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;

	// Unadvertise topic1 and publish a third message
	node.UnAdvertise(topic1);
	EXPECT_NE(node.Publish(topic1, data), 0);
	s_sleep(100);
	node.SpinOnce();
	EXPECT_FALSE(callbackExecuted);
}

//  ---------------------------------------------------------------------
/*TEST(DiscZmqTest, NPubSub)
{
	callbackExecuted = false;
	std::string master = "";
	bool verbose = false;
	std::string topic1 = "foo";
	std::string data = "someData";

	// Subscribe to topic1
	Node nodeSub(master, verbose);
	EXPECT_EQ(nodeSub.Subscribe(topic1, cb), 0);
	nodeSub.SpinOnce();

	// Advertise and publish some data on topic1
	Node *nodePub = new Node(master, verbose);
	EXPECT_EQ(nodePub->Advertise(topic1), 0);
	EXPECT_EQ(nodePub->Publish(topic1, data), 0);
	s_sleep(200);
	nodeSub.SpinOnce();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
	callbackExecuted = false;
	delete nodePub;

	// Publish a second message on topic1 with a new node
	nodePub = new Node(master, verbose);
	EXPECT_EQ(nodePub->Advertise(topic1), 0);
	EXPECT_EQ(nodePub->Publish(topic1, data), 0);
	s_sleep(100);
	nodeSub.SpinOnce();

	// Check that the data was received
	EXPECT_TRUE(callbackExecuted);
}*/

//  ---------------------------------------------------------------------
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
