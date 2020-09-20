#ifndef _MINI_TEST_H_
#define _MINI_TEST_H_
#include <string>
#include <sstream>
#include <iomanip>
#include <deque>

// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////
// Mini Unit Testing framework
// //////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////

/** \brief Unit testing framework.
 * \warning: all assertions are non-aborting, test case execution will continue
 *           even if an assertion namespace.
 *           This constraint is for portability: the framework needs to compile
 *           on Visual Studio 6 and must not require exception usage.
 */
namespace minitest {

class Failure {
public:
  const char* file_;
  unsigned int line_;
  std::string expr_;
  std::string message_;
  unsigned int nestingLevel_;
};

/// Context used to create the assertion callstack on failure.
/// Must be a POD to allow inline initialisation without stepping
/// into the debugger.
struct PredicateContext {
  using Id = unsigned int;
  Id id_;
  const char* file_;
  unsigned int line_;
  const char* expr_;
  PredicateContext* next_;
  /// Related Failure, set when the PredicateContext is converted
  /// into a Failure.
  Failure* failure_;
};

class TestResult {
public:
  TestResult();

  /// \internal Implementation detail for assertion macros
  /// Not encapsulated to prevent step into when debugging failed assertions
  /// Incremented by one on assertion predicate entry, decreased by one
  /// by addPredicateContext().
  PredicateContext::Id predicateId_{1};

  /// \internal Implementation detail for predicate macros
  PredicateContext* predicateStackTail_;

  void setTestName(const std::string& name);

  /// Adds an assertion failure.
  TestResult& addFailure(const char* file, unsigned int line,
                         const char* expr = nullptr);

  /// Removes the last PredicateContext added to the predicate stack
  /// chained list.
  /// Next messages will be targed at the PredicateContext that was removed.
  TestResult& popPredicateContext();

  bool failed() const;

  void printFailure(bool printTestName) const;

  // Generic operator that will work with anything ostream can deal with.
  template <typename T> TestResult& operator<<(const T& value) {
    std::stringstream oss;
    oss << std::setprecision(16) << std::hexfloat << value;
    return addToLastFailure(oss.str());
  }

  // Specialized versions.
  TestResult& operator<<(bool value);
  // std:ostream does not support 64bits integers on all STL implementation
  TestResult& operator<<(int64_t value);
  TestResult& operator<<(uint64_t value);

private:
  TestResult& addToLastFailure(const std::string& message);
  /// Adds a failure or a predicate context
  void addFailureInfo(const char* file, unsigned int line, const char* expr,
                      unsigned int nestingLevel);
  static std::string indentText(const std::string& text,
                                 const std::string& indent);

  using Failures = std::deque<Failure>;
  Failures failures_;
  std::string name_;
  PredicateContext rootPredicateNode_;
  PredicateContext::Id lastUsedPredicateId_{0};
  /// Failure which is the target of the messages added using operator <<
  Failure* messageTarget_{nullptr};
};

class TestCase {
public:
  TestCase();

  virtual ~TestCase();

  void run(TestResult& result);

  virtual const char* testName() const = 0;

protected:
  TestResult* result_{nullptr};

private:
  virtual void runTestCase() = 0;
};

/// Function pointer type for TestCase factory
using TestCaseFactory = TestCase* (*)();

class Runner {
public:
  Runner();

  /// Adds a test to the suite
  Runner& add(TestCaseFactory factory);

  /// Runs test as specified on the command-line
  /// If no command-line arguments are provided, run all tests.
  /// If --list-tests is provided, then print the list of all test cases
  /// If --test <testname> is provided, then run test testname.
  int runCommandLine(int argc, const char* argv[]) const;

  /// Runs all the test cases
  bool runAllTest(bool printSummary) const;

  /// Returns the number of test case in the suite
  size_t testCount() const;

  /// Returns the name of the test case at the specified index
  std::string testNameAt(size_t index) const;

  /// Runs the test case at the specified index using the specified TestResult
  void runTestAt(size_t index, TestResult& result) const;

  static void printUsage(const char* appName);

private: // prevents copy construction and assignment
  Runner(const Runner& other) = delete;
  Runner& operator=(const Runner& other) = delete;

private:
  void listTests() const;
  bool testIndex(const std::string& testName, size_t& indexOut) const;
  static void preventDialogOnCrash();

private:
  using Factories = std::deque<TestCaseFactory>;
  Factories tests_;
};

template <typename T, typename U>
TestResult& checkEqual(TestResult& result, T expected, U actual,
                       const char* file, unsigned int line, const char* expr) {
  if (static_cast<U>(expected) != actual) {
    result.addFailure(file, line, expr);
    result << "Expected: " << static_cast<U>(expected) << "\n";
    result << "Actual  : " << actual;
  }
  return result;
}

std::string ToJsonString(const char* toConvert);
std::string ToJsonString(std::string in);
#if JSONCPP_USING_SECURE_MEMORY
std::string ToJsonString(std::string in);
#endif

TestResult& checkStringEqual(TestResult& result, const std::string& expected,
                             const std::string& actual, const char* file,
                             unsigned int line, const char* expr);


/// \brief Asserts that the given expression is true.
/// JSONTEST_ASSERT( x == y ) << "x=" << x << ", y=" << y;
/// JSONTEST_ASSERT( x == y );
#define JSONTEST_ASSERT(expr)                                                  \
  if (expr) {                                                                  \
  } else                                                                       \
    result_->addFailure(__FILE__, __LINE__, #expr)

/// \brief Asserts that the given predicate is true.
/// The predicate may do other assertions and be a member function of the
/// fixture.
#define JSONTEST_ASSERT_PRED(expr)                                             \
  do {                                                                         \
    JsonTest::PredicateContext _minitest_Context = {                           \
        result_->predicateId_, __FILE__, __LINE__, #expr, NULL, NULL};         \
    result_->predicateStackTail_->next_ = &_minitest_Context;                  \
    result_->predicateId_ += 1;                                                \
    result_->predicateStackTail_ = &_minitest_Context;                         \
    (expr);                                                                    \
    result_->popPredicateContext();                                            \
  } while (0)

/// \brief Asserts that two values are equals.
#define MINITEST_ASSERT_EQUAL(expected, actual)                                \
  minitest::checkEqual(*result_, expected, actual, __FILE__, __LINE__,         \
                       #expected " == " #actual)

/// \brief Asserts that two values are equals.
#define MINITEST_ASSERT_STRING_EQUAL(expected, actual)                         \
  jsontest::checkStringEqual(*result_, JsonTest::ToJsonString(expected),       \
                             JsonTest::ToJsonString(actual), __FILE__,         \
                             __LINE__, #expected " == " #actual)

/// \brief Asserts that a given expression throws an exception
#define JSONTEST_ASSERT_THROWS(expr)                                           \
  do {                                                                         \
    bool _threw = false;                                                       \
    try {                                                                      \
      expr;                                                                    \
    } catch (...) {                                                            \
      _threw = true;                                                           \
    }                                                                          \
    if (!_threw)                                                               \
      result_->addFailure(__FILE__, __LINE__,                                  \
                          "expected exception thrown: " #expr);                \
  } while (0)

/// \brief Begin a fixture test case.
#define JSONTEST_FIXTURE(FixtureType, name)                                    \
  class Test##FixtureType##name : public FixtureType {                         \
  public:                                                                      \
    static minitest::TestCase* factory() {                                     \
      return new Test##FixtureType##name();                                    \
    }                                                                          \
                                                                               \
  public: /* overridden from TestCase */                                       \
    const char* testName() const override { return #FixtureType "/" #name; }   \
    void runTestCase() override;                                               \
  };                                                                           \
                                                                               \
  void Test##FixtureType##name::runTestCase()

#define JSONTEST_FIXTURE_FACTORY(FixtureType, name)                            \
  &Test##FixtureType##name::factory

#define JSONTEST_REGISTER_FIXTURE(runner, FixtureType, name)                   \
  (runner).add(JSONTEST_FIXTURE_FACTORY(FixtureType, name))

/// \brief Begin a fixture test case.
#define MINITEST_FIXTURE_V2(FixtureType, name, collections)                    \
  class Test##FixtureType##name : public FixtureType {                         \
  public:                                                                      \
    static minitest::TestCase* factory() {                                     \
      return new Test##FixtureType##name();                                    \
    }                                                                          \
    static bool collect() {                                                    \
      (collections).push_back(JSONTEST_FIXTURE_FACTORY(FixtureType, name));    \
      return true;                                                             \
    }                                                                          \
                                                                               \
  public: /* overridden from TestCase */                                       \
    const char* testName() const override { return #FixtureType "/" #name; }   \
    void runTestCase() override;                                               \
  };                                                                           \
                                                                               \
  static bool test##FixtureType##name##collect =                               \
      Test##FixtureType##name::collect();                                      \
                                                                               \
  void Test##FixtureType##name::runTestCase()


}






#endif
