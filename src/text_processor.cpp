#include "aiws/text_processor.hpp"
#include <algorithm>
#include <string>

namespace aiws {

    std::vector<TokenInfo> TextProcessor::tokenize(const std::string& text) {
        // TODO: produce normalized tokens with source and paragraph information.
        std::vector<TokenInfo> tokens = {};

        std::size_t paragraph = { 0 };

        bool previous_was_new_line = { false };
        bool only_space_or_tab_since_newline = { true };

        std::size_t i = { 0 };

        while (i < text.size())
        {

            if (text[i] == '\r' || text[i] == '\n')
            {
                if (previous_was_new_line && only_space_or_tab_since_newline)
                {
                    ++paragraph;
                }
                if (text[i] == '\r' && i + 1 < text.size() && text[i + 1] == '\n')
                {
                    i += 2;
                }
                else
                {
                    ++i;
                }
                previous_was_new_line = true;
                only_space_or_tab_since_newline = true;
                continue;
            }
        

            if (text[i] == ' ' || text[i] == '\t')
            {
                ++i;
                continue;
            }

            previous_was_new_line = false;
            only_space_or_tab_since_newline = false;

            unsigned char c = static_cast<unsigned char>(text[i]);

            bool valid = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');

            if (!valid)
            {
            
                ++i;
                continue;
            }



            std::size_t begin = { i };
            std::string token = {};

            while (i < text.size())
            {
                unsigned char current = static_cast<unsigned char>(text[i]);
                if (current >= 'A' && current <= 'Z')
                {
                    token += static_cast<char>(current - 'A' + 'a');
                }
                else if ((current >= 'a' && current <= 'z') || (current >= '0' && current <= '9'))
                {
                    token += static_cast<char>(current);
                }
                else
                {
                    break;
                }
                ++i;
            }

            tokens.push_back(TokenInfo{token, begin, i, paragraph });

        }


    return tokens;
}

std::vector<std::string> TextProcessor::terms(const std::string& text) {
    // TODO: return the normalized terms represented by the input text.
    std::vector<std::string> result = {};
    std::vector<TokenInfo> tokens = tokenize(text);

    for (const TokenInfo& token : tokens)
    {
        result.push_back(token.token);
    }
    return result;
}

std::string TextProcessor::normalize(const std::string& text) {
    // TODO: return the normalized form of the input text.

    std::vector<std::string> normalized_terms = terms(text);
    return join(normalized_terms, 0, normalized_terms.size());
}

std::string TextProcessor::join(const std::vector<TokenInfo>& tokens,
                                std::size_t begin,
                                std::size_t end) {
    // TODO: join the requested token range into normalized text.
    std::string result = {};


    end = std::min(end, tokens.size());

    for (std::size_t i = begin; i < end; ++i)
    {
        if (!result.empty())
        {
            result += ' ';
        }
        result += tokens[i].token;
    }

    return result;
}

std::string TextProcessor::join(const std::vector<std::string>& tokens,
                                std::size_t begin,
                                std::size_t end) {
    // TODO: join the requested term range into normalized text.

    std::string result = {};

    end = std::min(end, tokens.size());

    for (std::size_t i = begin; i < end; ++i)
    {
        if (!result.empty())
        {
            result += ' ';
        }
        result += tokens[i];

    }

    return result;
}

}  // namespace aiws
