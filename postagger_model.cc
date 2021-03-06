#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <regex>
#include "postagger_model.h"
#include "util.h"
#include "mltk/_ctagger.cc"

using namespace std;

Postagger_Model::~Postagger_Model(){
    if (postagger != nullptr) delete postagger;
    if (weights != nullptr) delete weights;
    if (specified_tags != nullptr) delete specified_tags;
    if (bias_weights != nullptr) delete bias_weights;
}

void Postagger_Model::load_pos_tagger(){
    log("start loading pos tagger");

    tagmap_in_t *specified_tags = new tagmap_in_t;
    ifstream st_fs("data/specified_tags.txt");
    string k, v;

    while (st_fs >> k >> v){
        (*specified_tags)[k] = v;
    }

    log("finish reading data/specified_tags.txt");

    class_weights_in_t *bias_weights = new class_weights_in_t;
    ifstream bw_fs("data/bias_weights.txt");
    string k2;
    float v2;

    while (bw_fs >> k2 >> v2){
        (*bias_weights).push_back(make_pair(k2, v2));
    }

    log("finish reading data/bias_weights.txt");

    weights_in_t *weights = new weights_in_t;
    ifstream w_fs("data/weights.txt");
    string k3; 

    while (w_fs.peek() != EOF){
        map<string, class_weights_in_t> m;
        string line;
        while (getline(w_fs, line)){
            istringstream iss(line);
            iss >> k3;

            if (k3 == "####"){
                (*weights).push_back(m);
                break;
            }

            class_weights_in_t vec;
            string v3_1;
            float v3_2;
            while (iss >> v3_1 >> v3_2){
                vec.push_back(make_pair(v3_1, v3_2));
            }

            m[k3] = vec;
        }
    }

    log("finish reading data/weights.txt");

    postagger = new PerceptronTagger(*weights, *bias_weights, *specified_tags);
    this->weights = weights;
    this->bias_weights = bias_weights;
    this->specified_tags = specified_tags;

    log("finish loading pos tagger");
}

string make_regex(string keyword){
    // regex special chars: [\^$.|?*+(){}
    ostringstream oss;
    for (int i = 0; i < keyword.length(); i += 1){
        switch (keyword[i]){
            case '[':
            case '\\':
            case '^':
            case '$':
            case '.':
            case '|':
            case '?':
            case '*':
            case '+':
            case '(':
            case ')':
            case '{':
            case '}':
                oss << "\\";
            default:
                oss << keyword[i];
                break;
        }
    }

    string escaped_keyword = oss.str();

    ostringstream oss2;
    oss2 << "[\\s|(|,](";
    oss2 << escaped_keyword;
    oss2 << ")[\\s|)|,|.]";
    return oss2.str();
}

bool valid (string keyword){
    return keyword != "e.g." && keyword != "E.g." && keyword != "E.G";
}

vector<pair<string, string> > Postagger_Model::tag_sentence(string str){
    for (int i = 0; i < str.length(); i += 1){
        if (str[i] == char(127)){
            str[i] = '\0';
        }
    }

    unordered_map<int, string> history;
    int i = 0;
    for (int j = 0; j < keyword_insertion.size(); j += 1){
        string keyword = keyword_insertion[j];
        regex e(make_regex(keyword), regex_constants::icase);
        smatch m;
        regex_search(str, m, e);

        // if match
        if (m.str(1) != "" && valid(m.str(1))){
            int start_index = m.position(1);
            int end_index = start_index + m.str(1).length() - 1;

            ostringstream oss;
            oss << char(127) << "_" << i;
            string replaced_k = oss.str();
            history[i++] = keyword;

            str.replace(start_index, end_index - start_index + 1, replaced_k);
        }
    }

    istringstream iss(str);
    vector<string> sentence;
    string word;
    int start_index = 0, end_index = -1;

    while (iss >> word){
        start_index = end_index + 1;
        end_index = start_index + word.length() - 1;

        if (word[0] == char(127)){
            string index = word.substr(2, word.length() - 2);
            istringstream iss(index);
            iss >> i;
            word = history[i];
        }

        sentence.push_back(word);
    }

    vector<tag_t> tagged_words = get_pos_tagger()->tag_sentence(sentence);

    return tagged_words;
}