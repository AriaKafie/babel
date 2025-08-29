
#include <gmpxx.h>
#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include <openssl/sha.h>

extern std::string_view alphabet;

struct FeistelPRP
{
    uint32_t N;         
    size_t   D = 256;   
    size_t   HALF = 128;
    size_t   rounds;    
    std::string key;    

    FeistelPRP(uint32_t N_, size_t rounds_, std::string key_)
        : N(N_), rounds(rounds_), key(std::move(key_))
    {
        if (N < 2) throw std::invalid_argument("N must be >= 2");
    }


    std::vector<uint32_t> to_digits(const mpz_class& x) const
    {
        std::vector<uint32_t> d(D, 0);
        mpz_class t = x;
        
        for (size_t i = 0; i < D; ++i)
        {
            unsigned long r = mpz_class(t % N).get_ui();
            d[i] = static_cast<uint32_t>(r);
            t /= N;
        }
        
        if (t != 0)
            throw std::invalid_argument("x >= N^D");
        
        return d;
    }


    mpz_class from_digits(const std::vector<uint32_t>& d) const
    {
        if (d.size() != D)
            throw std::invalid_argument("bad digit length");
        
        mpz_class x = 0, base = N;
        
        for (size_t i = D; i-- > 0;)
        {
            x *= base;
            x += d[i];
        }
        
        return x;
    }

    static std::vector<uint8_t> sha256(const std::vector<uint8_t>& in) {
        std::vector<uint8_t> out(32);
        SHA256_CTX ctx;
        SHA256_Init(&ctx);
        SHA256_Update(&ctx, in.data(), in.size());
        SHA256_Final(out.data(), &ctx);
        return out;
    }

    static std::vector<uint8_t> hkdf_stream(const std::vector<uint8_t>& seed, size_t need)
    {
        std::vector<uint8_t> out;
        out.reserve(need);
        uint64_t ctr = 0;
        
        while (out.size() < need)
        {
            std::vector<uint8_t> block(seed);
            for (int i = 0; i < 8; ++i) block.push_back(uint8_t((ctr >> (8*i)) & 0xFF));
            auto h = sha256(block);
            size_t take = std::min(h.size(), need - out.size());
            out.insert(out.end(), h.begin(), h.begin() + take);
            ++ctr;
        }
        
        return out;
    }

    void bytes_to_digits_modN(const std::vector<uint8_t>& bytes,
                              std::vector<uint32_t>& out_digits) const
    {
        size_t need = out_digits.size();

        size_t idx = 0;
        auto get16 = [&](size_t k)->uint32_t{
            uint32_t v = bytes[k];
            if (k + 1 < bytes.size()) v |= (uint32_t(bytes[k+1]) << 8);
            return v;
        };
        
        size_t k = 0;
        
        while (idx < need)
        {
            if (k + 1 >= bytes.size()) break;
            uint32_t v = get16(k);
            out_digits[idx++] = v % N;
            k += 2;
        }

        for (; idx < need; out_digits[idx++] = 0);
    }


    void F(const std::vector<uint32_t>& in_digits,
           size_t round_idx,
           std::vector<uint32_t>& out_digits) const
    {

        std::vector<uint8_t> seed;

        for (int i = 0; i < 8; ++i) seed.push_back(uint8_t((round_idx >> (8*i)) & 0xFF));
        seed.insert(seed.end(), key.begin(), key.end());

        seed.reserve(seed.size() + in_digits.size()*2);
        for (auto v : in_digits) { seed.push_back(uint8_t(v & 0xFF)); seed.push_back(uint8_t(v >> 8)); }


        auto stream = hkdf_stream(seed, HALF * 2);
        bytes_to_digits_modN(stream, out_digits);
    }


    mpz_class permute(const mpz_class& x) const {
        auto d = to_digits(x);
        std::vector<uint32_t> L(d.begin(), d.begin() + HALF);
        std::vector<uint32_t> R(d.begin() + HALF, d.end());
        std::vector<uint32_t> T(HALF), Fout(HALF);

        for (size_t r = 0; r < rounds; ++r) {

            F(R, r, Fout);

            for (size_t i = 0; i < HALF; ++i) {
                uint32_t t = L[i] + Fout[i];
                if (t >= N) t -= N;
                T[i] = t;
            }

            L.swap(R);
            R.swap(T);
        }

        d.assign(L.begin(), L.end());
        d.insert(d.end(), R.begin(), R.end());
        return from_digits(d);
    }

    mpz_class invert(const mpz_class& y) const {
        auto d = to_digits(y);
        std::vector<uint32_t> L(d.begin(), d.begin() + HALF);
        std::vector<uint32_t> R(d.begin() + HALF, d.end());
        std::vector<uint32_t> T(HALF), Fout(HALF);

        for (size_t r = rounds; r-- > 0; ) {

            F(L, r, Fout);
            for (size_t i = 0; i < HALF; ++i) {
                uint32_t v = (R[i] + N - Fout[i]);
                if (v >= N) v -= N;
                T[i] = v; 
            }
            R.swap(L);  
            L.swap(T);  
        }
        d.assign(L.begin(), L.end());
        d.insert(d.end(), R.begin(), R.end());
        return from_digits(d);
    }
};

mpz_class permute(mpz_class m)
{

    uint32_t N = alphabet.size();
    size_t rounds = 12;
    std::string key = "your-secret-key";

    FeistelPRP prp(N, rounds, key);

    mpz_class p = prp.permute(m);

    return p;
}

mpz_class invert(mpz_class m)
{
    uint32_t N = alphabet.size();
    size_t rounds = 12;
    std::string key = "your-secret-key";

    FeistelPRP prp(N, rounds, key);

    mpz_class x = prp.invert(m);

    return x;
}
