// E:\CodeLve\src\llm\mock_llm.h
#pragma once
#include "llm_interface.h"
#include <chrono>
#include <thread>
#include <random>
#include <vector>
#include <string>

namespace codelve {
    namespace llm {

        /**
         * Mock LLM implementation for testing without an actual model.
         * Simulates responses with configurable delay to mimic real behavior.
         */
        class MockLlm {
        public:
            /**
             * Generate a mock response based on the prompt.
             * @param prompt The input prompt
             * @param streamingCallback Callback for streaming tokens
             * @return Whether generation was successful
             */
            static bool generateResponse(
                const std::string& prompt,
                ResponseCallback streamingCallback) {

                // Parse the prompt to detect query type
                bool isCodeQuery = prompt.find("code") != std::string::npos ||
                    prompt.find("function") != std::string::npos ||
                    prompt.find("class") != std::string::npos;

                // Generate appropriate response based on query type
                std::string response;
                if (prompt.find("/help") != std::string::npos) {
                    response = generateHelpResponse();
                }
                else if (isCodeQuery) {
                    response = generateCodeResponse(prompt);
                }
                else {
                    response = generateGeneralResponse(prompt);
                }

                // Stream the response token by token with realistic timing
                streamTokens(response, streamingCallback);
                return true;
            }

        private:
            static std::string generateHelpResponse() {
                return "# CodeLve Help\n\n"
                    "CodeLve is an offline code analysis tool powered by Code Llama.\n\n"
                    "## Available Commands:\n"
                    "- `/help` - Display this help message\n"
                    "- `/clear` - Clear the conversation history\n"
                    "- `/info` - Show system information\n\n"
                    "## Usage Tips:\n"
                    "1. Load a code repository using the file browser\n"
                    "2. Ask questions about the code\n"
                    "3. Request explanations, documentation, or refactoring suggestions\n\n"
                    "For more information, visit the CodeLve documentation.";
            }

            static std::string generateCodeResponse(const std::string& prompt) {
                return "I've analyzed the code you're referencing. Here's what I found:\n\n"
                    "The code appears to be implementing a core component with several key functions. "
                    "Let me break down how it works:\n\n"
                    "1. The main functionality is centered around processing user inputs and generating appropriate responses.\n"
                    "2. There's a context management system that tracks the conversation state.\n"
                    "3. The code uses standard libraries for file I/O, string manipulation, and data structures.\n\n"
                    "If you have specific questions about certain functions or want to optimize parts of the code, "
                    "I can help with that too. Let me know what aspects you'd like me to focus on in more detail.";
            }

            static std::string generateGeneralResponse(const std::string& prompt) {
                return "Thanks for your question. As a code analysis assistant, I can help you understand "
                    "codebases, debug issues, and suggest improvements.\n\n"
                    "For the best results, try loading a code repository using the file browser and "
                    "asking specific questions about the code. I can also help with general programming "
                    "concepts and best practices.\n\n"
                    "If you'd like to see what I can do, try asking me to explain a function, suggest "
                    "optimizations, or help with debugging an issue in your code.";
            }

            static void streamTokens(const std::string& response, ResponseCallback callback) {
                // Simulate token-by-token generation
                std::vector<std::string> tokens;

                // Simple tokenization by words and punctuation
                std::string current;
                for (char c : response) {
                    if (c == ' ' || c == '\n' || c == '.' || c == ',' || c == ':' || c == ';') {
                        if (!current.empty()) {
                            tokens.push_back(current);
                            current.clear();
                        }
                        tokens.push_back(std::string(1, c));
                    }
                    else {
                        current += c;
                    }
                }
                if (!current.empty()) {
                    tokens.push_back(current);
                }

                // Random generator for timing variations
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> delay_dist(30, 100);  // 30-100ms between tokens

                // Stream tokens with slight randomized delays
                for (size_t i = 0; i < tokens.size(); i++) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
                    callback(tokens[i], i == tokens.size() - 1);
                }
            }
        };

    }
} // namespace codelve::llm