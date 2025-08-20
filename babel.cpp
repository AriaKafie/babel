
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <iostream>
#include <string>
#include <fstream>
#include <cmath>
#include <gmpxx.h>
#include <regex>

#include "feistel.h"

constexpr size_t len = 256;

std::string_view alphabet = "abcdefghijklmnopqrstuvwxyz,. ";

std::string alphabetize(mpz_class n)
{
    std::string result(len, alphabet[0]);
    
    for (int i = result.size() - 1; i >= 0 && n; i--)
    {
        mpz_class mod = n % alphabet.size();
        result[i] = alphabet[mod.get_ui()];
        
        n /= alphabet.size();
    }

    return result;
}

mpz_class de_alphabetize(const std::string& text)
{
    mpz_class base(alphabet.size());
    mpz_class result(0);

    for (int i = 0, j = text.size() - 1; j >= 0; i++, j--)
    {
        mpz_class temp;
        mpz_pow_ui(temp.get_mpz_t(), base.get_mpz_t(), i);

        mpz_class digit_value(alphabet.find(text[j]));

        result += temp * digit_value;
    }

    return result;
}

std::string to_formatted_page(const std::string& text)
{
    std::stringstream ss;
    
    for (int i = 0; i < 8; i++)
    {
        for (int j = 0; j < 32; j++)
            ss << text[i * 32 + j];

        ss << "\n";
    }

    return ss.str();
}

void browse(mpz_class page_num)
{
    std::string opt;

    do
    {
        std::cout << "[n]ext page, [p]revious page, or [b]ack to search?" << std::endl;
        std::getline(std::cin, opt);

        if (opt == "n") page_num++;
        if (opt == "p") page_num--;
        
        if (opt == "n" || opt == "p")
        {
            std::cout << "On page " << page_num << ":\n" << std::endl
                      << to_formatted_page(alphabetize(permute(page_num - 1))) << std::endl;
        }
        
    } while (opt != "b");
}

int main()
{
    srand((unsigned int)time(NULL));
    
    for (;;)
    {
        std::string opt;
        
        do
        {
            std::cout << "Welcome to the book of babel. [s]earch for text or [f]lip to a page?" << std::endl;
            std::getline(std::cin, opt);

        } while (opt != "s" && opt != "f");

        if (opt == "f")
        {
            std::string page_num;
            
            do
            {
                std::cout << "What page? (1 -> " << alphabet.size() << "^" << len << ")" << std::endl;
                std::getline(std::cin, page_num);
                
            } while (!std::regex_match(page_num, std::regex("[1-9]\\d*")));

            mpz_class p(page_num);
            mpz_class permutation = permute(p - 1);

            std::cout << "\n" << to_formatted_page(alphabetize(permutation)) << std::endl;

            browse(p);
            
            continue;
        }

        std::string text;
        
        do
        {
            std::cout << "Enter some text (up to " << len << " characters)" << std::endl;
            std::getline(std::cin, text);

        } while (text.find_first_not_of(alphabet) != std::string::npos);

        text = text.substr(0, len);
        
        std::stringstream exact;
        exact << std::left << std::setw(len) << text;
        std::string exact_match = exact.str();

        std::string random_match;

        for (int i = 0; i < len; i++)
            random_match += alphabet[rand() % alphabet.size()];

        for (int i = (len - text.size()) / 2, j = 0; j < text.size(); i++, j++)
            random_match[i] = text[j];

        mpz_class exact_de_alphabetized  = de_alphabetize(exact_match);
        mpz_class random_de_alphabetized = de_alphabetize(random_match);
        mpz_class exact_page_num         = invert(exact_de_alphabetized) + 1;
        mpz_class random_page_num        = invert(random_de_alphabetized) + 1;

        std::cout << "\nRandom match found on page " << random_page_num << ":\n\n"
                  << to_formatted_page(alphabetize(random_de_alphabetized)) << std::endl;
        
        std::cout << "Exact match found on page " << exact_page_num << ":\n\n"
                  << to_formatted_page(alphabetize(exact_de_alphabetized)) << std::endl;

        browse(exact_page_num);
    }
    
    return 0;
}
