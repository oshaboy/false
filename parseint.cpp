
#include <unicode/uchar.h>
#include <unicode/schriter.h>
#include <array>
#include <algorithm>
#include "false.h"
static const std::array<char32_t,68> naughts={
	U'0', /* Western */
	U'٠', /* Arabic */
	U'۰', /* Farsi */
	U'߀', /* N'Ko */
	U'०', /* Devenagari */
	U'০', /* Bengali */
	U'੦', /*Gurmukhi*/
	U'૦', /*Gujarati*/
	U'୦', /*Oriya*/
	U'௦', /*Tamil*/
	U'౦', /*Telugu*/
	U'೦', /*Kannada*/
	U'൦', /*Malayam*/
	U'෦', /*Sinhala*/
	U'๐', /*Thai*/
	U'໐', /*Lao*/
	U'༠', /*Tibetian*/
	U'၀', /*Myanmar*/
	U'႐', /*Shan*/
	U'០', /*Khmer*/
	U'᠐', /*Mongolian*/
	U'᥆', /*Limbu*/
	U'᧐', /*Thai Lue*/
	U'᪀', /*Thai Tham Hora*/
	U'᪐', /*Thai Tham Tham*/
	U'᭐', /*Balinese*/
	U'᮰', /*Sudanese*/
	U'᱀', /*Lepcha*/
	U'᱐', /*Ol Chiki*/
	U'꘠', /*Vai*/
	U'꣐', /*Saurashtra*/
	U'꤀', /*Kayah Li*/
	U'꧐', /*Javanese*/
	U'꧰', /*Thai Laing*/
	u'꩐', /*Cham*/
	U'꯰', /*Meetei Mayek*/
	U'０', /* Fullwidth */
	U'𐒠', /*Osmanya*/
	U'𐴰',/*Rohingya*/
	U'𑁦', /*Brahmi*/
	U'𑃰', /*Sora Sompeng*/
	U'𑄶', /*Chakma*/
	U'𑇐', /*Sharada*/
	U'𑋰', /*Khudawadi*/
	U'𑑐', /*Newa*/
	U'𑓐', /*Tirhuta*/
	U'𑙐', /*Modi*/
	U'𑛀', /*Tahkri*/
	U'𑜰', /*Ahom*/
	U'𑣠', /*Warang Citi*/
	U'𑥐', /*Dives Akuru*/
	U'𑱐', /*Bhaisuki*/
	U'𑵐', /*Mahsaram Gondi*/
	U'𑶠', /*Gunjala Gondi*/
	U'𑽐', /*Kawi*/
	U'𖩠', /*Mro*/
	U'𖫀', /*Tangsa*/
	U'𖭐', /*Pahawh Hmong*/
	U'𝟎', /*Bold*/
	U'𝟘', /*Double Struck*/
	U'𝟢', /*Sans Serif*/
	U'𝟬', /*Sans Serif Bold*/
	U'𝟶', /*Monospace*/
	U'𞅀',/*Nyiakeng Puachue Hmong*/
	U'𞋰', /*Wancho*/
	U'𞓰', /*Nag Mundari*/
	U'𞥐', /*Adlam*/
	U'🯰' /* Seven Segment */
};

bool uni_isdigit(char32_t c){
	std::array<char32_t,naughts.size()> differences;
    std::transform(naughts.begin(), naughts.end(), differences.begin(), [c](char32_t c2){return ((char32_t)c)-c2;});
    return *std::min_element(differences.begin(), differences.end())<10u;
}
unsigned long long State::parse_long_unicode(){
    unsigned long long accumulator=0;
    while(curchar_iter<curline.end()){
		char32_t c=*curchar_iter;
		std::array<char32_t,naughts.size()> differences;
		std::transform(naughts.begin(), naughts.end(), differences.begin(), [c](char32_t c2){return c-c2;});
        char32_t dig=*std::min_element(differences.begin(), differences.end());
        if (dig >= 10) break;
        accumulator*=10;
        accumulator+=dig;
		this->nextChar();
    }
    return accumulator;
    
}
inline std::optional<char32_t> nextChar(std::u32string::const_iterator iter, std::u32string::const_iterator end, size_t& count){
	if (iter<end){
		count++;
		return *(iter++);
	} else {
		return std::nullopt;
	}
}
unsigned long long parse_long_unicode(std::u32string::const_iterator iter, std::u32string::const_iterator end, size_t& count){
    unsigned long long accumulator=0;
    while(iter<end){
		char32_t c=*iter;
		std::array<char32_t,naughts.size()> differences;
		std::transform(naughts.begin(), naughts.end(), differences.begin(), [c](char32_t c2){return c-c2;});
        char32_t dig=*std::min_element(differences.begin(), differences.end());
        if (dig >= 10) break;
        accumulator*=10;
        accumulator+=dig;
		nextChar(iter,end, count);
    }
    return accumulator;
    
}