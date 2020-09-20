#include "MiniTest.h"


namespace minitest {
// class TestResult
// //////////////////////////////////////////////////////////////////

TestResult::TestResult() {
  // The root predicate has id 0
  rootPredicateNode_.id_ = 0;
  rootPredicateNode_.next_ = nullptr;
  predicateStackTail_ = &rootPredicateNode_;
}

void TestResult::setTestName(const std::string& name) { name_ = name; }

TestResult& TestResult::addFailure(const char* file, unsigned int line,
                                   const char* expr) {
  /// Walks the PredicateContext stack adding them to failures_ if not already
  /// added.
  unsigned int nestingLevel = 0;
  PredicateContext* lastNode = rootPredicateNode_.next_;
  for (; lastNode != nullptr; lastNode = lastNode->next_) {
    if (lastNode->id_ > lastUsedPredicateId_) // new PredicateContext
    {
      lastUsedPredicateId_ = lastNode->id_;
      addFailureInfo(lastNode->file_, lastNode->line_, lastNode->expr_,
                     nestingLevel);
      // Link the PredicateContext to the failure for message target when
      // popping the PredicateContext.
      lastNode->failure_ = &(failures_.back());
    }
    ++nestingLevel;
  }

  // Adds the failed assertion
  addFailureInfo(file, line, expr, nestingLevel);
  messageTarget_ = &(failures_.back());
  return *this;
}

void TestResult::addFailureInfo(const char* file, unsigned int line,
                                const char* expr, unsigned int nestingLevel) {
  Failure failure;
  failure.file_ = file;
  failure.line_ = line;
  if (expr) {
    failure.expr_ = expr;
  }
  failure.nestingLevel_ = nestingLevel;
  failures_.push_back(failure);
}

TestResult& TestResult::popPredicateContext() {
  PredicateContext* lastNode = &rootPredicateNode_;
  while (lastNode->next_ != nullptr && lastNode->next_->next_ != nullptr) {
    lastNode = lastNode->next_;
  }
  // Set message target to popped failure
  PredicateContext* tail = lastNode->next_;
  if (tail != nullptr && tail->failure_ != nullptr) {
    messageTarget_ = tail->failure_;
  }
  // Remove tail from list
  predicateStackTail_ = lastNode;
  lastNode->next_ = nullptr;
  return *this;
}

bool TestResult::failed() const { return !failures_.empty(); }

void TestResult::printFailure(bool printTestName) const {
  if (failures_.empty()) {
    return;
  }

  if (printTestName) {
    printf("* Detail of %s test failure:\n", name_.c_str());
  }

  // Print in reverse to display the callstack in the right order
  for (const auto& failure : failures_) {
    std::string indent(failure.nestingLevel_ * 2, ' ');
    if (failure.file_) {
      printf("%s%s(%u): ", indent.c_str(), failure.file_, failure.line_);
    }
    if (!failure.expr_.empty()) {
      printf("%s\n", failure.expr_.c_str());
    } else if (failure.file_) {
      printf("\n");
    }
    if (!failure.message_.empty()) {
      std::string reindented = indentText(failure.message_, indent + "  ");
      printf("%s\n", reindented.c_str());
    }
  }
}

std::string TestResult::indentText(const std::string& text,
                                    const std::string& indent) {
  std::string reindented;
  std::string::size_type lastIndex = 0;
  while (lastIndex < text.size()) {
    std::string::size_type nextIndex = text.find('\n', lastIndex);
    if (nextIndex == std::string::npos) {
      nextIndex = text.size() - 1;
    }
    reindented += indent;
    reindented += text.substr(lastIndex, nextIndex - lastIndex + 1);
    lastIndex = nextIndex + 1;
  }
  return reindented;
}

TestResult& TestResult::addToLastFailure(const std::string& message) {
  if (messageTarget_ != nullptr) {
    messageTarget_->message_ += message;
  }
  return *this;
}

TestResult& TestResult::operator<<(int64_t value) {
  return addToLastFailure(std::to_string(value));
}

TestResult& TestResult::operator<<(uint64_t value) {
  return addToLastFailure(std::to_string(value));
}

TestResult& TestResult::operator<<(bool value) {
  return addToLastFailure(value ? "true" : "false");
}

// class TestCase
// //////////////////////////////////////////////////////////////////

TestCase::TestCase() = default;

TestCase::~TestCase() = default;

void TestCase::run(TestResult& result) {
  result_ = &result;
  runTestCase();
}

// class Runner
// //////////////////////////////////////////////////////////////////

Runner::Runner() = default;

Runner& Runner::add(TestCaseFactory factory) {
  tests_.push_back(factory);
  return *this;
}

size_t Runner::testCount() const { return tests_.size(); }

std::string Runner::testNameAt(size_t index) const {
  TestCase* test = tests_[index]();
  std::string name = test->testName();
  delete test;
  return name;
}

void Runner::runTestAt(size_t index, TestResult& result) const {
  TestCase* test = tests_[index]();
  result.setTestName(test->testName());
  printf("Testing %s: ", test->testName());
  fflush(stdout);
#if JSON_USE_EXCEPTION
  try {
#endif // if JSON_USE_EXCEPTION
    test->run(result);
#if JSON_USE_EXCEPTION
  } catch (const std::exception& e) {
    result.addFailure(__FILE__, __LINE__, "Unexpected exception caught:")
        << e.what();
  }
#endif // if JSON_USE_EXCEPTION
  delete test;
  const char* status = result.failed() ? "FAILED" : "OK";
  printf("%s\n", status);
  fflush(stdout);
}

bool Runner::runAllTest(bool printSummary) const {
  size_t const count = testCount();
  std::deque<TestResult> failures;
  for (size_t index = 0; index < count; ++index) {
    TestResult result;
    runTestAt(index, result);
    if (result.failed()) {
      failures.push_back(result);
    }
  }

  if (failures.empty()) {
    if (printSummary) {
      printf("All %zu tests passed\n", count);
    }
    return true;
  }
  for (auto& result : failures) {
    result.printFailure(count > 1);
  }

  if (printSummary) {
    size_t const failedCount = failures.size();
    size_t const passedCount = count - failedCount;
    printf("%zu/%zu tests passed (%zu failure(s))\n", passedCount, count,
           failedCount);
  }
  return false;
}

bool Runner::testIndex(const std::string& testName, size_t& indexOut) const {
  const size_t count = testCount();
  for (size_t index = 0; index < count; ++index) {
    if (testNameAt(index) == testName) {
      indexOut = index;
      return true;
    }
  }
  return false;
}


}
