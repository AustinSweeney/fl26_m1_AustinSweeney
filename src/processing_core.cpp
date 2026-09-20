#include "aiws/processing_core.hpp"
#include "aiws/chunker.hpp"
#include "aiws/context_builder.hpp"
#include "aiws/corpus_index.hpp"
#include "aiws/retrieval_engine.hpp"
#include "aiws/text_processor.hpp"

#include <stdexcept>
#include <vector>
#include <string>
#include <unordered_set>
#include <utility>


namespace aiws {

struct ProcessingCore::Impl {
    // TODO: define the internal state used by the processing core.
    std::vector<Chunk> chunks;
    CorpusIndex index;
    RetrievalEngine retrieval;
    ContextBuilder context_builder;
};

ProcessingCore::ProcessingCore() : impl_(std::make_unique<Impl>()) { }

ProcessingCore::~ProcessingCore() = default;

ProcessingCore::ProcessingCore(ProcessingCore&&) noexcept = default;

ProcessingCore& ProcessingCore::operator=(ProcessingCore&&) noexcept = default;

std::string ProcessingCore::normalize(const std::string& text) {
    // TODO: return the normalized form of the input text.

    return TextProcessor::normalize(text);
}

void ProcessingCore::rebuild(const Workspace& workspace) {
    // TODO: rebuild the processing state from the workspace.
    std::unordered_set<std::string> document_ids;

    for (const Document& document : workspace.documents())
    {
        if (!document_ids.insert(document.id()).second)
        {
            throw std::invalid_argument("duplicate document ids");
        }
    }
    ChunkingPolicy policy{};

    policy.max_tokens = kMaxChunkTokens;
    policy.overlap = kChunkOverlap;
    policy.paragraph_window = kParagraphPreferenceWindow;

    Chunker chunker(policy);

    std::vector<Chunk> new_chunks;

    const std::vector<Document>& documents = workspace.documents();

    for (std::size_t i = 0; i < documents.size(); ++i)
    {
        std::vector<Chunk> document_chunks = chunker.chunk(documents[i], i);

        new_chunks.insert(new_chunks.end(), document_chunks.begin(), document_chunks.end());

    }
    CorpusIndex new_index(new_chunks);

    impl_->chunks = std::move(new_chunks);
    impl_->index = std::move(new_index);

}

const std::vector<Chunk>& ProcessingCore::chunks() const noexcept {
  

    // TODO: return the chunks currently stored by the processing core.
    return impl_->chunks;
}

std::size_t ProcessingCore::chunk_count() const noexcept {
    // TODO: return the number of stored chunks.
    return impl_->chunks.size();
}

std::size_t ProcessingCore::document_frequency(const std::string& term) const {
    // TODO: return the document frequency for the requested term.
    std::vector<std::string> terms = TextProcessor::terms(term);

    if (terms.empty())
    {
        return 0;
    }
    if (terms.size() > 1)
    {
        throw std::invalid_argument("terms must normalized to exactly 1 token");
    }
    return impl_->index.document_frequency(terms[0]);
}

std::size_t ProcessingCore::term_frequency(const std::string& term,
                                           const std::string& chunk_id) const {
    // TODO: return the term frequency for the requested chunk.
    std::vector<std::string> terms = TextProcessor::terms(term);

    if (terms.empty())
    {
        return 0;
    }
    if (terms.size() > 1)
    {
        throw std::invalid_argument("Terms must normalize to 1 token");
    }

    return impl_->index.term_frequency(terms[0], chunk_id);
}

std::vector<SearchResult> ProcessingCore::search(const std::string& query, int k) const {
    // TODO: return the ranked results for the requested query.
    
    return impl_->retrieval.search(query, k, impl_->chunks, impl_->index);
}

std::vector<ContextItem> ProcessingCore::build_context(const std::string& query,
                                                       int k,
                                                       std::size_t token_budget) const {
    // TODO: build bounded context for the requested query.
    if (token_budget == 0)
    {
        return {};
    }

    std::vector<SearchResult> ranked = search(query, k);

    return impl_->context_builder.build(ranked, token_budget);


}

}  // namespace aiws
