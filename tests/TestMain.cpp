#include <juce_core/juce_core.h>
#include <juce_events/juce_events.h>
#include <iostream>

/**
    ANTI-MATR test runner (JUCE UnitTest based).

    Usage: AntiMatrTests [category-or-test-name] [--seed N]
*/
int main (int argc, char* argv[])
{
    juce::ArgumentList args (argc, argv);
    juce::ScopedJuceInitialiser_GUI init;

    juce::UnitTestRunner runner;
    runner.setAssertOnFailure (false);
    runner.setPassesAreLogged (false);

    juce::int64 seed = args.containsOption ("--seed") ? args.getValueForOption ("--seed").getLargeIntValue() : 0;

    juce::String filter;
    for (int i = 1; i < argc; ++i)
    {
        const juce::String a (argv[i]);
        if (! a.startsWith ("--")) filter = a;
    }

    if (filter.isNotEmpty())
    {
        juce::Array<juce::UnitTest*> tests;
        for (auto* t : juce::UnitTest::getAllTests())
            if (t->getCategory().equalsIgnoreCase (filter) || t->getName().containsIgnoreCase (filter))
                tests.add (t);
        if (tests.isEmpty()) { std::cerr << "No tests match '" << filter << "'" << std::endl; return 2; }
        runner.runTests (tests, seed);
    }
    else
    {
        runner.runAllTests (seed);
    }

    int failures = 0, passes = 0;
    for (int i = 0; i < runner.getNumResults(); ++i)
    {
        const auto* r = runner.getResult (i);
        failures += r->failures;
        passes += r->passes;
        if (r->failures > 0)
        {
            std::cerr << "FAIL: " << r->unitTestName << " / " << r->subcategoryName << std::endl;
            for (const auto& m : r->messages) std::cerr << "   " << m << std::endl;
        }
    }
    std::cout << "ANTI-MATR tests: " << passes << " passed, " << failures << " failed" << std::endl;
    return failures == 0 ? 0 : 1;
}
