
#include "gtest/gtest.h"

// Fake lgraph class for testing
class LGraph {
public:
  int id;

  LGraph() {
    static int conta=0;
    id = conta++;
  };

  int get_id() const { return id; };
};

#include "eprp.hpp"

static bool is_equal_called = false;

class test1{
public:
  static void foo(Eprp_var &var) {
    fmt::print("test1.foo");
    for (const auto& v : var.dict) {
      fmt::print(" {}:{}",v.first,v.second);
    }
    fmt::print(" ::");
    for (const auto& v : var.lgs) {
      fmt::print(" {}",v->get_id());
    }
    fmt::print("\n");

    var.add("test1_foo","field1");
    EXPECT_STREQ(var.get("test1_foo").c_str(), "field1");
  }

  static void is_equal(Eprp_var &var) {
    EXPECT_STREQ(var.get("test1_foo").c_str(), "field1");

    EXPECT_EQ(var.get("lgdb"), var.get("check1"));

    EXPECT_EQ(var.get("graph_name"), var.get("check2"));

    EXPECT_STRNE(var.get("lgdb").c_str(), "");

    fmt::print("var.get = {}\n", var.get("nofield2"));

    EXPECT_STREQ(var.get("nofield").c_str(), "");

    is_equal_called = true;
  }
  static void pass(Eprp_var &var) {
  }
};

class test2{
public:
  static void bar(Eprp_var &var) {
    fmt::print("test1.foo");
    for (const auto& v : var.dict) {
      fmt::print(" {}:{}",v.first,v.second);
    }
    fmt::print(" ::");
    for (const auto& v : var.lgs) {
      fmt::print(" {}",v->get_id());
    }
    fmt::print("\n");

    var.add("test2_bar", "field2");
    var.add(new LGraph());
  }
};

class EPrpTest : public ::testing::Test {
protected:
  Eprp eprp;
  void SetUp() override {
    Eprp_method m1("test1.xyz.generate", "Generate a random test/method call to foo", &test1::foo);
    m1.add_label_required("lgdb","lgraph directory");
    m1.add_label_optional("graph_name","another super duper attribute");

    Eprp_method m2("test1.fff.test","fff::is_equal call", &test1::is_equal);
    m2.add_label_required("lgdb","lgraph directory");
    m2.add_label_required("check1","check1 super duper attribute");
    // Missing check2 attribute on purpose to check warning generation
    //
    Eprp_method m3("test1.pass", "pass value through", &test1::pass);

    eprp.register_method(m1);
    eprp.register_method(m2);
    eprp.register_method(m3);
  }
};

TEST_F(EPrpTest, SimpleReadlinePipe) {
  is_equal_called = false;
  const char *buffer =" test1.xyz.generate lgdb:./lgdb graph_name:chacha |> test1.fff.test     check2:chacha    check1:./lgdb   ";

  eprp.parse("inline", buffer, strlen(buffer));

  EXPECT_TRUE(is_equal_called);
  is_equal_called=false;

  buffer =" test1.pass check2:chacha  check1:./lgdb  lgdb:./lgdb |> @a";
  eprp.parse("inline", buffer, strlen(buffer));

  buffer ="@a |> test1.fff.test";
  eprp.parse("inline", buffer, strlen(buffer));
  EXPECT_TRUE(is_equal_called);

}
