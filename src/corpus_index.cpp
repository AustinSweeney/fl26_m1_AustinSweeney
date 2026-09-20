#include "aiws/corpus_index.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <string>
#include <vector>
#include <unordered_map>

namespace aiws {

CorpusIndex::CorpusIndex(const std::vector<Chunk>& chunks) {
    build(chunks);
}

void CorpusIndex::build(const std::vector<Chunk>& chunks) {
    // TODO: build the searchable index from the supplied chunks.
    postings_.clear();
    chunk_by_id_.clear();

    for (std::size_t i = 0; i < chunks.size(); ++i)
    {
        const Chunk& chunk = chunks[i];
        chunk_by_id_[chunk.id] = i;

        std::vector<std::string> terms = TextProcessor::terms(chunk.text);

        std::unordered_map<std::string, std::size_t> frequencies;

        for (const std::string& term : terms)
        {
            ++frequencies[term];
        }

        for (const auto& entry : frequencies)
        {
            postings_[entry.first].push_back(Posting{ i, entry.second });
        }
    }
}

std::size_t CorpusIndex::document_frequency(
    const std::string& normalized_term) const noexcept {
    // TODO: return how many chunks contain the requested term.

    auto it = postings_.find(normalized_term);

    if (it == postings_.end())
    {
        return 0;
    }
    return it->second.size();
}

std::size_t CorpusIndex::term_frequency(
    const std::string& normalized_term,
    const std::string& chunk_id) const noexcept {
    // TODO: return the requested term's frequency in the specified chunk.

    auto chunk_it = chunk_by_id_.find(chunk_id);

    if (chunk_it == chunk_by_id_.end())
    {
        return 0;
    }

    auto posting_it = postings_.find(normalized_term);

    if (posting_it == postings_.end())
    {
        return 0;
    }

    

    std::size_t index = chunk_it->second;

    for (const Posting& posting : posting_it->second)
    {
        if (posting.chunk_index == index)
        {
            return posting.frequency;
        }
    }
    return 0;
}

const std::vector<CorpusIndex::Posting>* CorpusIndex::postings(
    const std::string& normalized_term) const noexcept {
    // TODO: return the postings associated with the requested term.
    auto it = postings_.find(normalized_term);

    if (it == postings_.end())
    {
        return nullptr;
    }
    return &it->second;
}

const Chunk* CorpusIndex::find_chunk(
    const std::vector<Chunk>& chunks,
    const std::string& chunk_id) const noexcept {
    // TODO: find the chunk identified by the requested chunk ID.

    auto it = chunk_by_id_.find(chunk_id);

    if (it == chunk_by_id_.end())
    {
        return nullptr;
    }

    std::size_t index = it->second;

    if (index >= chunks.size())
    {
        return nullptr;
    }
    return &chunks[index];
}

std::size_t CorpusIndex::chunk_index(const std::string& chunk_id) const {
    // TODO: return the stored index of the requested chunk ID.

    auto it = chunk_by_id_.find(chunk_id);

    if (it == chunk_by_id_.end())
    {
        throw std::out_of_range("chunk id does not exist");
    }
    return it->second;
}

}  // namespace aiws
