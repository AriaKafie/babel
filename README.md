## Overview
The Book of Babel is a toy “infinite library” generator written in C++. It maps every 256-character page over a small alphabet (`"abcdefghijklmnopqrstuvwxyz,. "`) to a unique page number, and vice-versa, using a Feistel-based length-preserving permutation. You can:
- **flip** to any page number and see its content,
- **search** for a string and jump to the page where it appears exactly (and a random page that contains it).

## Building
Dependencies:
- GMP / GMPXX (big integers)
- OpenSSL (SHA-256)

Compile with the provided rule:
```bash
make
# or manually:
g++ babel.cpp feistel.cpp -o babel -lgmpxx -lgmp -lcrypto -w
```
## Sample usage
```
Welcome to the book of babel. [s]earch for text or [f]lip to a page?
s
Enter some text (up to 256 characters)
hello, world.

Random match found on page 126372465991610335788123093476261239707067163600354487952874655402737243126495156293653997617956479544959277285898443361259441784104306076455774555445794870887759654593814657589097465160069595151828845240378001199918126208651540189019473830599057115499664885560728112837823861313287604302385787151199344712700175833341832526723277752442358826148130216280041722761634311388257:

hfoif esxuudwlu.lnhsbuadnikxchbr
aykvtfjlwgprojgsutxxr.wvvfclkfco
ddomnfhcd.gavc, sbmbpu u.ee ibiz
co.hlfpdgoc.vltenpqvhflqghello,
world.w xcwytabxja,rbpmbwsap, we
kvhv dfk fzk.fqo,safbzeztxwogznz
cajx.jc.hus.kx,s,xqinqx  pj,vgdc
o.,hyrwzuwjmelfabefo.ddeyyb,rfo,

Exact match found on page 214150023818311406582420601420949391462010170713257047222771077155087249650996005912606828157725967705543157807210245210209409624474018789796736923273719547049329228697827819306555727118854137101351096248232829940284560731728784050257885330170613602366721424395962672035914539750132776304713530748840262443154568818234677386638155860639045700446470307784915524421901370106853:

hello, world.








[n]ext page, [p]revious page, or [b]ack to search?
n
On page 214150023818311406582420601420949391462010170713257047222771077155087249650996005912606828157725967705543157807210245210209409624474018789796736923273719547049329228697827819306555727118854137101351096248232829940284560731728784050257885330170613602366721424395962672035914539750132776304713530748840262443154568818234677386638155860639045700446470307784915524421901370106854:

,rpgdvioio.thqwseucyaopwkbnjsvpj
zrsrn knqqp,dwhgskx.ywvfx.lv,d,u
pwt vazcubu hetiedrmykrup,a,skkw
gayelxjg,xyuashx zoydpypxtdoo,h.
ygjb.rwsqkzcujfn nujnnt.sxkddgys
gatothr.qyd.wqjlfuasq,vmbzxaqzzd
epuzmtnnzmddvjiklcix,wmivwjuwbrq
vhe.lifpnycto ecvebbdpcqfdn.ayqo

[n]ext page, [p]revious page, or [b]ack to search?
p
On page 214150023818311406582420601420949391462010170713257047222771077155087249650996005912606828157725967705543157807210245210209409624474018789796736923273719547049329228697827819306555727118854137101351096248232829940284560731728784050257885330170613602366721424395962672035914539750132776304713530748840262443154568818234677386638155860639045700446470307784915524421901370106853:

hello, world.








[n]ext page, [p]revious page, or [b]ack to search?
b
Welcome to the book of babel. [s]earch for text or [f]lip to a page?
f
What page? (1 -> 29^256)
214150023818311406582420601420949391462010170713257047222771077155087249650996005912606828157725967705543157807210245210209409624474018789796736923273719547049329228697827819306555727118854137101351096248232829940284560731728784050257885330170613602366721424395962672035914539750132776304713530748840262443154568818234677386638155860639045700446470307784915524421901370106853

hello, world.








[n]ext page, [p]revious page, or [b]ack to search?
```

## How it works (high level)
- **Alphabetization:** interprets a page as a base-29 number and back:
  - `alphabetize(n) → 256-char string`
  - `de_alphabetize(text) → big integer`
- **Permutation:** a Feistel PRP on 256 base-29 digits provides a bijection over all pages:
  - `permute(x)` maps the index to a shuffled index,
  - `invert(y)` is the exact inverse.
- **Formatting:** pages are displayed as 8 lines × 32 characters.

## Interesting bits
- **Fixed-length PRP over base-N digits.** The content space is `N^D` with `N=29`, `D=256`. The Feistel network shuffles digit vectors without changing length or alphabet.
- **Round function from SHA-256.** Each round derives pseudo-random digits from `(round_idx, key, right_half)` via a tiny HKDF-like stream built on SHA-256.
- **Deterministic library.** With a fixed `key` and `rounds`, every page number deterministically maps to the same page forever.

### Code snippets

Permutation/inversion are standard Feistel transforms over base-29 digits:
```cpp
mpz_class permute(const mpz_class& x) const {
    auto d = to_digits(x);                 // base-29, 256 digits
    std::vector<uint32_t> L(d.begin(), d.begin() + HALF);
    std::vector<uint32_t> R(d.begin() + HALF, d.end());
    std::vector<uint32_t> T(HALF), Fout(HALF);

    for (size_t r = 0; r < rounds; ++r) {
        F(R, r, Fout);                     // SHA-256 based stream → digits mod N
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
```

Alphabetization and de-alphabetization:
```cpp
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
```
