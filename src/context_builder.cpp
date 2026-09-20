#include "aiws/context_builder.hpp"
#include "aiws/text_processor.hpp"

#include <algorithm>
#include <string>
#include <unordered_set>
#include <vector>

namespace aiws {

std::vector<ContextItem> ContextBuilder::build(const std::vector<SearchResult>& ranked, std::size_t token_budget) const {
    // TODO: assemble ranked results into context items within the supplied token budget.
    

    std::vector<ContextItem> context = {};

    if (token_budget == 0)
    {
        return context;
    }

    std::size_t remaining = token_budget;

    std::unordered_set<std::string> included;

    for (const SearchResult& result : ranked)
    {
        if (!included.insert(result.chunk_id).second)
        {
            continue;
        }
        std::vector<std::string> tokens = TextProcessor::terms(result.text);

        if (tokens.empty())
        {
            continue;
        }

        ContextItem item = {};

        item.chunk_id = result.chunk_id;
        item.document_id = result.document_id;
        item.chunk_sequence = result.chunk_sequence;
        item.score = result.score;

        if (tokens.size() <= remaining)
        {
            item.text = TextProcessor::join(tokens, 0, tokens.size());

            item.token_count = tokens.size();
            item.truncated = false;

            context.push_back(item);

            remaining -= tokens.size();

            if (remaining == 0)
            {
                break;
            }
        }
        else
            {
             item.text = TextProcessor::join(tokens, 0, remaining);
             item.token_count = remaining;
             item.truncated = true;

             context.push_back(item);
             break;
        }
        
    }
    return context;
}

}  // namespace aiws
