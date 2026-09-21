#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <algorithm>
#include <cmath>
#include <set>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <string>

namespace aiws {

double RetrievalEngine::canonical_score(double value) {
    // TODO: return the score in the required canonical form.
    return std::round(value * 1e12) / 1e12;
}

std::vector<SearchResult> RetrievalEngine::search(const std::string& query,
                                                  int k,
                                                  const std::vector<Chunk>& chunks,
                                                  const CorpusIndex& index) const {
    // TODO: return the ranked search results for the requested query.
    if (k < 0)
    {
        throw std::invalid_argument("k can't be negative");
    }

    if (k == 0 || chunks.empty())
    {
        return {};
    }

    std::vector<std::string> query_terms = TextProcessor::terms(query);

    if (query_terms.empty())
    {
        return {};
    }

    std::unordered_set<std::string> seen_terms = {};
    std::vector<std::string> unique_terms = {};

    for (const std::string& term : query_terms)
    {
        if (seen_terms.insert(term).second)
        {
            unique_terms.push_back(term);
        }

    }

    struct ScoreData {
        double base_score{ 0.0 };
        std::size_t matched_terms{0};
    };
    std::vector<ScoreData> scores(chunks.size());

    const double N = static_cast<double>(chunks.size());

    for (const std::string& term : unique_terms)
    {
        const std::vector<CorpusIndex::Posting>* postings = index.postings(term);


        if (postings == nullptr)
        {
            continue;
        }

        const double df = static_cast<double>(index.document_frequency(term));

        const double idf = std::log((N + 1.0) / (df + 1.0)) + 1.0;


        for (const CorpusIndex::Posting& posting : *postings)
        {
            const double tf = 1.0 + std::log(static_cast<double>(posting.frequency));

            scores[posting.chunk_index].base_score += tf * idf;
            ++scores[posting.chunk_index].matched_terms;
        }
    }
    std::vector<SearchResult> results = {};

    const double query_term_count = static_cast<double>(unique_terms.size());

    for (std::size_t i = 0; i < chunks.size(); ++i)
    {
        if (scores[i].matched_terms == 0)
        {
            continue;
        }
        const double coverage = 1.0 + 0.1 * static_cast<double>(scores[i].matched_terms) / query_term_count;

        const double final_score = canonical_score(scores[i].base_score * coverage);

        SearchResult result = {};

        result.chunk_id = chunks[i].id;
        result.document_id = chunks[i].document_id;
        result.chunk_sequence = chunks[i].sequence;
        result.text = chunks[i].text;
        result.score = final_score;
        result.matched_terms = scores[i].matched_terms;

        results.push_back(result);
    }
    

    //this sort was sourced from AI information in Ai log
    std::sort(
        results.begin(),
        results.end(),
        [&index](const SearchResult& a,
                 const SearchResult& b)
        {
            if (a.score != b.score)
            {
                return a.score > b.score;
            }

            std::size_t a_index =
                index.chunk_index(a.chunk_id);

            std::size_t b_index =
                index.chunk_index(b.chunk_id);

            return a_index < b_index;
        });
    //end of AI assistance
    if (results.size() > static_cast<std::size_t>(k))
    {
        results.resize(static_cast<std::size_t>(k));
    }
    
    return results;
}

}  // namespace aiws

