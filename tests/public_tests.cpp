#include "aiws/processing_core.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

    int failures = 0;

    void check(bool condition, const std::string& message) {
        if (!condition) {
            ++failures;
            std::cerr << "FAIL: " << message << '\n';
        }
    }

    std::string numbered_words(int n) {
        std::string s;

        for (int i = 0; i < n; ++i) {
            if (!s.empty()) {
                s += ' ';
            }

            s += "w" + std::to_string(i);
        }

        return s;
    }

}

int main() {
    using namespace aiws;

    check(
        ProcessingCore::normalize("Hello, WORLD!") == "hello world",
        "normalization converts ASCII letters to lowercase");

    check(
        ProcessingCore::normalize("R2-D2") == "r2 d2",
        "punctuation separates tokens");

    check(
        ProcessingCore::normalize("123 456") == "123 456",
        "digits are retained");

    check(
        ProcessingCore::normalize("!!!...---").empty(),
        "separator-only text normalizes to empty");

    check(
        ProcessingCore::normalize("one\t\tTWO\nthree") == "one two three",
        "whitespace separates normalized tokens");

    Workspace empty_ws;
    empty_ws.add_document(Document{ "empty", "Empty", "!!!" });

    ProcessingCore core;
    core.rebuild(empty_ws);

    check(
        core.chunk_count() == 0,
        "effectively empty document produces no chunks");

    Workspace exact_ws;
    exact_ws.add_document(
        Document{ "exact", "Exact", numbered_words(120) });

    core.rebuild(exact_ws);

    check(
        core.chunk_count() == 1,
        "exactly 120 tokens produces one chunk");

    check(
        !core.chunks().empty() &&
        core.chunks()[0].token_count == 120,
        "120-token chunk has correct token count");

    Workspace overlap_ws;
    overlap_ws.add_document(
        Document{ "overlap", "Overlap", numbered_words(121) });

    core.rebuild(overlap_ws);

    check(
        core.chunk_count() == 2,
        "121 tokens produces two chunks");

    check(
        core.chunks().size() == 2 &&
        core.chunks()[0].token_count == 120 &&
        core.chunks()[1].token_count == 21,
        "second chunk includes 20-token overlap");

    check(
        core.chunks().size() == 2 &&
        core.chunks()[0].id == "overlap#0" &&
        core.chunks()[1].id == "overlap#1",
        "chunk IDs contain document ID and sequence");

    Workspace index_ws;

    index_ws.add_document(
        Document{ "a", "A", "apple apple banana" });

    index_ws.add_document(
        Document{ "b", "B", "apple orange" });

    core.rebuild(index_ws);

    check(
        core.document_frequency("APPLE!") == 2,
        "document frequency normalizes input");

    check(
        core.document_frequency("missing") == 0,
        "missing term has document frequency zero");

    check(
        core.term_frequency("apple", "a#0") == 2,
        "term frequency counts repeated terms");

    check(
        core.term_frequency("banana", "b#0") == 0,
        "term not present in chunk has frequency zero");

    check(
        core.term_frequency("apple", "missing#0") == 0,
        "missing chunk ID has frequency zero");

    check(
        core.document_frequency("!!!") == 0,
        "term normalizing to zero tokens has frequency zero");

    bool multi_term_threw = false;

    try {
        (void)core.document_frequency("apple banana");
    }
    catch (const std::invalid_argument&) {
        multi_term_threw = true;
    }

    check(
        multi_term_threw,
        "multiple normalized frequency terms throw invalid_argument");

    Workspace replacement_ws;
    replacement_ws.add_document(
        Document{ "new", "New", "zebra zebra" });

    core.rebuild(replacement_ws);

    check(
        core.chunk_count() == 1,
        "rebuild replaces previous chunks");

    check(
        core.document_frequency("apple") == 0,
        "rebuild removes old index data");

    check(
        core.document_frequency("zebra") == 1,
        "rebuild indexes new corpus");

    Workspace duplicate_ws;

    duplicate_ws.add_document(
        Document{ "dup", "First", "one two" });

    duplicate_ws.add_document(
        Document{ "dup", "Second", "three four" });

    bool duplicate_threw = false;

    try {
        core.rebuild(duplicate_ws);
    }
    catch (const std::invalid_argument&) {
        duplicate_threw = true;
    }

    check(
        duplicate_threw,
        "duplicate document IDs throw invalid_argument");

    check(
        core.document_frequency("zebra") == 1,
        "failed rebuild preserves previous corpus");

    Workspace search_ws;

    search_ws.add_document(
        Document{ "first", "First", "alpha beta" });

    search_ws.add_document(
        Document{ "second", "Second", "alpha gamma" });

    core.rebuild(search_ws);

    auto results = core.search("alpha beta", 10);

    check(
        results.size() == 2,
        "search returns chunks containing at least one query term");

    check(
        !results.empty() &&
        results[0].document_id == "first",
        "chunk matching more query terms ranks first");

    auto duplicate_query =
        core.search("alpha alpha beta beta", 10);

    check(
        duplicate_query.size() == results.size(),
        "duplicate query terms do not create extra candidates");

    check(
        !duplicate_query.empty() &&
        duplicate_query[0].document_id == "first",
        "duplicate query terms do not change top result");

    check(
        core.search("", 10).empty(),
        "empty query produces no results");

    check(
        core.search("!!!", 10).empty(),
        "separator-only query produces no results");

    check(
        core.search("alpha", 0).empty(),
        "k zero produces no results");

    bool negative_k_threw = false;

    try {
        (void)core.search("alpha", -1);
    }
    catch (const std::invalid_argument&) {
        negative_k_threw = true;
    }

    check(
        negative_k_threw,
        "negative k throws invalid_argument");

    Workspace tie_ws;

    tie_ws.add_document(
        Document{ "first", "First", "same" });

    tie_ws.add_document(
        Document{ "second", "Second", "same" });

    core.rebuild(tie_ws);

    auto ties = core.search("same", 10);

    check(
        ties.size() == 2,
        "tie search returns both chunks");

    check(
        ties.size() == 2 &&
        ties[0].document_id == "first" &&
        ties[1].document_id == "second",
        "score ties preserve document insertion order");

    Workspace context_ws;

    context_ws.add_document(
        Document{ "ctx", "Context",
                 "alpha beta gamma delta epsilon" });

    core.rebuild(context_ws);

    auto zero_context =
        core.build_context("alpha", 10, 0);

    check(
        zero_context.empty(),
        "zero token budget produces empty context");

    auto full_context =
        core.build_context("alpha", 10, 5);

    check(
        full_context.size() == 1 &&
        full_context[0].token_count == 5 &&
        !full_context[0].truncated,
        "whole chunk is included when it fits budget");

    auto truncated_context =
        core.build_context("alpha", 10, 3);

    check(
        truncated_context.size() == 1 &&
        truncated_context[0].token_count == 3 &&
        truncated_context[0].truncated,
        "context truncates chunk to remaining token budget");

    check(
        !truncated_context.empty() &&
        truncated_context[0].text == "alpha beta gamma",
        "truncated context contains largest fitting prefix");

    Workspace end_to_end;

    end_to_end.add_document(
        Document{ "doc1", "Document 1",
                 "robot drone navigation" });

    end_to_end.add_document(
        Document{ "doc2", "Document 2",
                 "robot rover mapping" });

    end_to_end.add_document(
        Document{ "doc3", "Document 3",
                 "database storage system" });

    core.rebuild(end_to_end);

    auto end_results =
        core.search("robot navigation", 10);

    check(
        end_results.size() == 2,
        "multi-document search finds candidate union");

    check(
        !end_results.empty() &&
        end_results[0].document_id == "doc1",
        "multi-document ranking prefers stronger match");

    if (failures == 0) {
        std::cout << "All student tests passed.\n";
        return 0;
    }

    std::cerr << failures << " student test(s) failed.\n";
    return 1;
}