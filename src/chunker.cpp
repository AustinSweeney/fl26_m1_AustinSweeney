#include "aiws/chunker.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <algorithm>
#include <string>
#include <vector>

namespace aiws {

Chunker::Chunker(ChunkingPolicy policy) : policy_(policy) {
    if (policy_.max_tokens == 0 || policy_.overlap >= policy_.max_tokens ||
        policy_.paragraph_window > policy_.max_tokens) {
        throw std::invalid_argument("invalid chunking policy");
    }
}

std::vector<Chunk> Chunker::chunk(const Document& document, std::size_t document_order) const {
    // TODO: produce deterministic, source-attributed chunks for the supplied document.
    
    std::vector<Chunk> chunks = {};

    std::vector<TokenInfo> tokens = TextProcessor::tokenize(document.text());

    if (tokens.empty())
    {
        return chunks;
    }

    std::size_t start = { 0 };
    std::size_t sequence = { 0 };

    while (start < tokens.size())
    {
        std::size_t remaining = tokens.size() - start;

        std::size_t end = std::min(start + policy_.max_tokens, tokens.size());

        if (remaining > policy_.max_tokens)
        {
            std::size_t window_start = start + policy_.max_tokens - policy_.paragraph_window;

            std::size_t preferred_end = end;

            for (std::size_t i = window_start; i < end; ++i)
            {
                if (i > start && i < tokens.size() && tokens[i].paragraph != tokens[i - 1].paragraph)
                {
                    preferred_end = i;
                }
            }
            end = preferred_end;
        }

        Chunk current = {};

        current.id = document.id() + "#" + std::to_string(sequence);

        current.document_id = document.id();
        current.document_order = document_order;
        current.sequence = sequence;
        current.text = TextProcessor::join(tokens, start, end);

        current.token_count = end - start;

        current.source_begin = tokens[start].begin;
        current.source_end = tokens[end - 1].end;

        chunks.push_back(current);

       if (end == tokens.size())
       {
           break;
       }

       start = end - policy_.overlap;

       ++sequence;
     
    }
    
    return chunks;
}

}  // namespace aiws
