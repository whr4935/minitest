#include "../MiniTest.h"
#include <vector>

using namespace minitest;

static std::vector<minitest::TestCaseFactory> local_;

class ValueTest : public TestCase
{
protected:
    void failure() {
        int a = 1 + 2;
        MINITEST_ASSERT_EQUAL(a, 2);
    }
};


MINITEST_FIXTURE_V2(ValueTest, ValueTest1, local_)
{
    int a = 1 + 1;
    MINITEST_ASSERT_EQUAL(a, 1);

    failure(); 

    printf("hello world!\n");
}

int main(int argc, char *argv[])
{
    minitest::Runner runner;

    for (auto& p : local_) {
        runner.add(p);
    }

    runner.runAllTest(true);

    return 0;
}
