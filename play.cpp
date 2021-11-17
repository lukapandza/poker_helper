#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <numeric>
#include <map>
using namespace std;

#define DWORD int32_t
int HR[32487834];
float PROBS[2366];

string suits[] = {"C", "D", "H", "S"};
string card_nums[] = {"2", "3", "4", "5", "6", "7", "8", "9", "T", "J", "Q", "K", "A"};

map<string, int> SUIT_MAP;
map<string, int> RANK_MAP;

vector<int> DECK;

int NUM_SAMPLES = 250000;

unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
std::default_random_engine RNG(seed);

string now_to_string()
{
  chrono::system_clock::time_point p = chrono::system_clock::now();
  time_t t = chrono::system_clock::to_time_t(p);
  char str[26];
  ctime_s(str, sizeof str, &t);
  return str;
}

inline string int_code_to_str(int card) {
    return card_nums[(card - 1) / 4] + suits[(card - 1) % 4];
}

inline int str_to_int_code(string card) {
    return 4 * RANK_MAP[card.substr(0, 1)] + SUIT_MAP[card.substr(1, 1)] + 1;
}

int str_to_rank(string card) {
    string num_str = card.substr(0, 1);
    for (int i = 0; i <= 12; i++)
        if (card_nums[i] == num_str) return i;
    return 13; // shouldn't reach this unless error. add handler later!
}

inline bool are_same_suit(string c1, string c2) {
    return c1.substr(1, 1) == c2.substr(1, 1);
}

void print_hand(int* cards, int size) {
    string out = "( ";
    for(int i = 0; i < size; i++) {
        out += int_code_to_str(cards[i]) + " ";
    }
    out += ")\n";
    std::cout << out;
}

void print_hand(vector<int>& cards, int size = 0) {
    if (size == 0) {
        size = cards.size();
    }
    string out = "( ";
    for(int i = 0; i < size; i++) {
        out += int_code_to_str(cards[i]) + " ";
    }
    out += ")\n";
    std::cout << out;
}

void print_deck(vector<int>& deck) {
    std::cout << "[ ";
    for (int i = 0; i < deck.size(); i++) {
        std::cout << int_code_to_str(deck[i]) << ", ";
    }
    std::cout << "]\n";
}

int lookup_hand(int* hand) {

    int p = HR[53 + *hand++];
    p = HR[p + *hand++];
    p = HR[p + *hand++];
    p = HR[p + *hand++];
    p = HR[p + *hand++];
    p = HR[p + *hand++];
	int ret = HR[p + *hand++];

    return ret;
}

float pre_flop_win_estimate(vector<int>& my_cards, int num_opponents) {

    int c1 = (my_cards[0] - 1) / 4;
    int c2 = (my_cards[1] - 1) / 4;
    int s = (((my_cards[0] - 1) % 4) == ((my_cards[1] - 1) % 4)) ? 1 : 0; // are same suit?

    return PROBS[338 * (num_opponents - 1) + 26 * c1 + 2 * c2 + s];
}

int estimate_num_wins(int* my_hand, int* sample_hand, vector<int> sample_deck, int num_out_cards, int start_num_opponents, int num_opponents, int num_samples = 10000) {

    int win_count = 0;
    float avg_prob = 1.0f / (start_num_opponents + 1);
    std::normal_distribution<float> dist(0, .15f * avg_prob);

    for (int i = 0; i < num_samples; i++) {

        // shuffle deck
        shuffle(begin(sample_deck), end(sample_deck), RNG);

        int draw_index = 0;

        // draw to complete out_cards
        if (num_out_cards == 3) {
            my_hand[5] = sample_hand[3] = sample_deck[draw_index++];
            my_hand[6] = sample_hand[4] = sample_deck[draw_index++];
        }
        else if (num_out_cards == 4)
            my_hand[6] = sample_hand[4] = sample_deck[draw_index++];

        // score my_hand
        int my_score = lookup_hand(my_hand);
        int my_category = my_score >> 12;
        int my_strength = my_score & 0x00000FFF;

        bool is_best_hand = true;

        vector<int> possible_hand;
        possible_hand.push_back(0);
        possible_hand.push_back(0);
        

        for (int i = 0; i < num_opponents; i++) {

            possible_hand[0] = sample_deck[draw_index];
            possible_hand[1] = sample_deck[draw_index + 1];

            while(pre_flop_win_estimate(possible_hand, start_num_opponents) < (avg_prob + dist(RNG))) {

                shuffle(sample_deck.begin() + draw_index, sample_deck.end(), RNG);
                possible_hand[0] = sample_deck[draw_index];
                possible_hand[1] = sample_deck[draw_index + 1];
            }

            sample_hand[5] = sample_deck[draw_index++];
            sample_hand[6] = sample_deck[draw_index++];

            // score sample_hand
            int sample_score = lookup_hand(sample_hand);
            int sample_category = sample_score >> 12;
            int sample_strength = sample_score & 0x00000FFF;

            if((sample_category > my_category) || ((sample_category == my_category) && (sample_strength >= my_strength))) {
                is_best_hand = false; 
                break;
            }
        }
        
        if (is_best_hand) 
            win_count++;
    }

    return win_count;
}

float win_estimate(vector<int>& my_cards, vector<int>& out_cards, int start_num_opponents, int num_opponents, int num_samples = 100000) {

    vector<int> curr_deck(DECK);
    int num_out_cards = out_cards.size();

    //create my hand:
    int* my_hand = new int[7];
    my_hand[0] = my_cards[0];
    my_hand[1] = my_cards[1];

    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), my_cards[0]), curr_deck.end());
    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), my_cards[1]), curr_deck.end());

    // create sample_hand:
    int* sample_hand = new int[7];
    
    for (int i = 0; i < num_out_cards; i++) {
        my_hand[i + 2] = sample_hand[i] = out_cards[i];
        curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), out_cards[i]), curr_deck.end());
    }

    int win_count = estimate_num_wins(my_hand, sample_hand, curr_deck, num_out_cards, start_num_opponents, num_opponents, num_samples);

    delete[] my_hand;
    delete[] sample_hand;

    return float(win_count) / float(num_samples);
}

void test(vector<int>& my_cards, vector<int>& out_cards, vector<int> nums_opponents = {1, 3, 5}, vector<int> nums_samples = {1000, 10000, 100000, 1000000}, int num_tests = 32) {

    std::cout << " HAND => ";
    print_hand(my_cards, 2);
    std::cout << "CARDS => ";
    print_hand(out_cards, 3);
    std::cout << endl << endl;

    for (auto num_opponents : nums_opponents) {

        std::cout << "\n\n# opponents: " << num_opponents << endl;

        for (auto num_samples : nums_samples) {

            std::cout << "\n    # samples: " << num_samples << endl;

            vector<float> probs;
            vector<int> times;
            
            float min_prob = 1.0;
            float max_prob = 0.0;

            int min_time = 60000000;
            int max_time = 0;

            for (int test = 0; test < num_tests; test++) {

                auto start = chrono::high_resolution_clock::now();
                float p = win_estimate(my_cards, out_cards, num_opponents, num_opponents, num_samples);
                auto stop = chrono::high_resolution_clock::now();
                auto duration = chrono::duration_cast<chrono::microseconds>(stop - start);
                int t = duration.count();

                probs.push_back(p);
                times.push_back(t);

                if (p < min_prob) min_prob = p;
                if (p > max_prob) max_prob = p;

                if (t < min_time) min_time = t;
                if (t > max_time) max_time = t;
            }

            float mean_prob = accumulate(probs.begin(), probs.end(), 0.0) / num_tests;
            float mean_time = accumulate(times.begin(), times.end(), 0.0) / num_tests;

            std::cout << "        avg P(win) | " << mean_prob << endl;
            std::cout << "        err P(win) | " << "+/- " << (max_prob - min_prob) / 2 << endl;
            std::cout << "      range P(win) | [ " << min_prob << ", " << max_prob << " ]" << endl << endl;

            std::cout << "          avg time | " << mean_time << endl;
            std::cout << "          err time | " << "+/- " << (max_time - min_time) / 2 << endl;
            std::cout << "        range time | [ " << min_time << ", " << max_time << " ]" << endl;
        }
    }
}

void populate_maps() {

    SUIT_MAP["C"] = 0;
    SUIT_MAP["c"] = 0;
    SUIT_MAP["D"] = 1;
    SUIT_MAP["d"] = 1;
    SUIT_MAP["H"] = 2;
    SUIT_MAP["h"] = 2;
    SUIT_MAP["S"] = 3;
    SUIT_MAP["s"] = 3;

    RANK_MAP["2"] = 0;
    RANK_MAP["3"] = 1;
    RANK_MAP["4"] = 2;
    RANK_MAP["5"] = 3;
    RANK_MAP["6"] = 4;
    RANK_MAP["7"] = 5;
    RANK_MAP["8"] = 6;
    RANK_MAP["9"] = 7;
    RANK_MAP["T"] = 8;
    RANK_MAP["t"] = 8;
    RANK_MAP["J"] = 9;
    RANK_MAP["j"] = 9;
    RANK_MAP["Q"] = 10;
    RANK_MAP["q"] = 10;
    RANK_MAP["K"] = 11;
    RANK_MAP["k"] = 11;
    RANK_MAP["A"] = 12;
    RANK_MAP["a"] = 12;

}

int process_input_preflop(string input, vector<int>& my_cards, vector<int>& curr_deck) {

    if(input == "end") return -1; // break hand signal
    if(input == "exit") return -2; // close program signal

    int c1 = str_to_int_code(input.substr(0, 2));
    int c2 = str_to_int_code(input.substr(2, 2));

    if ((c1 < 1 || c1 > 52) || (c2 < 1 || c2 > 52)) return 1; // invalid input signal

    my_cards.push_back(c1);
    my_cards.push_back(c2);

    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c1), curr_deck.end());
    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c2), curr_deck.end());

    return 0; // default signal
}

int process_input_flop(string input, vector<int>& out_cards, vector<int>& curr_deck) {

    if(input == "end") return -1; // break hand signal
    if(input == "exit") return -2; // close program signal

    int c1 = str_to_int_code(input.substr(0, 2));
    int c2 = str_to_int_code(input.substr(2, 2));
    int c3 = str_to_int_code(input.substr(4, 2));

    if ((c1 < 1 || c1 > 52) || (c2 < 1 || c2 > 52) || (c3 < 1 || c3 > 52)) return 1; // invalid input signal

    out_cards.push_back(c1);
    out_cards.push_back(c2);
    out_cards.push_back(c3);

    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c1), curr_deck.end());
    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c2), curr_deck.end());
    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c3), curr_deck.end());

    return 0; // default signal
}

int process_input_turn(string input, vector<int>& out_cards, vector<int>& curr_deck) {

    if(input == "end") return -1; // break hand signal
    if(input == "exit") return -2; // close program signal

    int c1 = str_to_int_code(input.substr(0, 2));

    if (c1 < 1 || c1 > 52) return 1; // invalid input signal

    out_cards.push_back(c1);

    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c1), curr_deck.end());

    return 0; // default signal
}

int process_input_river(string input, vector<int>& out_cards, vector<int>& curr_deck) {

    if(input == "end") return -1; // break hand signal
    if(input == "exit") return -2; // close program signal

    int c1 = str_to_int_code(input.substr(0, 2));

    if (c1 < 1 || c1 > 52) return 1; // invalid input signal

    out_cards.push_back(c1);

    curr_deck.erase(std::remove(curr_deck.begin(), curr_deck.end(), c1), curr_deck.end());

    return 0; // default signal
}

int process_stack_size_input(ofstream& record_file, float& stack) {

    string input;

    std::cout << "Stack size: ";
    cin >> input;

    if(input == "end") return -1; // break hand signal
    if(input == "exit") return -2; // close program signal

    stack = std::stof(input);

    record_file << stack << endl;

    return 0; // default signal
}



int main(int argc, char* argv[]) {

	// Load the HandRanks.DAT file and map it into the HR array
	printf("Loading HandRanks.DAT file...");
	memset(HR, 0, sizeof(HR));
	FILE * fin = fopen("HandRanks.dat", "rb");
	if (!fin)
	    return false;
	size_t bytesread = fread(HR, sizeof(HR), 1, fin);	// get the HandRank Array
	fclose(fin);
	printf("complete.\n\n");

    // Load the PreFlopProbs.DAT file and map it into the HR array
	printf("\n\nLoading PreFlopProbs.DAT file...");
	memset(PROBS, 0, sizeof(PROBS));
	FILE * pfin = fopen("PreFlopProbs.dat", "rb");
	if (!pfin)
	    return false;
	size_t pbytesread = fread(PROBS, sizeof(float), 2366, pfin);	// get the HandRank Array
	fclose(pfin);
	printf("complete.\n\n");

    populate_maps();

    // generate deck:
    for (int i = 1; i < 53; i++) {
        DECK.push_back(i);
    }

    if (string(argv[1]) == "test") {

        string my_cards_str = string(argv[2]);
        string out_cards_str = string(argv[3]);

        vector<int> my_cards;
        my_cards.push_back(str_to_int_code(my_cards_str.substr(0, 2)));
        my_cards.push_back(str_to_int_code(my_cards_str.substr(2, 2)));

        vector<int> out_cards;
        out_cards.push_back(str_to_int_code(out_cards_str.substr(0, 2)));
        out_cards.push_back(str_to_int_code(out_cards_str.substr(2, 2)));
        out_cards.push_back(str_to_int_code(out_cards_str.substr(4, 2)));

        test(my_cards, out_cards);

        return 0;
    }

    bool should_record = false;
    if (string(argv[1]) == "record") {
        should_record = true;
    }

    ofstream record_file;

    if (should_record) {
        string curr = now_to_string();
        curr.erase( curr.end() - 1 );
        curr.replace(3, 1, "-");
        curr.replace(7, 1, "-");
        curr.replace(10, 1, "-");
        curr.replace(13, 1, "-");
        curr.replace(16, 1, "-");
        curr.replace(19, 1, "-");
        const string record_file_name = "./records/record-" + curr + ".txt";
        std::cout << record_file_name << endl;
        record_file.open(record_file_name);
    }

    bool is_new_hand = true;
    float stack;
    float big_blind;
    float start_stack;
    int hand_count = 0;
    
    if (should_record) {
        std::cout << "Big Blind: ";
        cin >> big_blind;
        record_file << big_blind << endl; // first line of record always big blind
        std::cout << endl << "Starting Stack: ";
        cin >> start_stack;
        record_file << start_stack << endl; // second line is starting amount
    }

    while (is_new_hand) {

        hand_count++;

        //declare variables:
        int signal = 1;
        string in_hand;
        string in_flop;
        string in_turn;
        string in_river;
        int num_opponents;
        int start_num_opponents;
        float pot;

        vector<int> my_cards;
        vector<int> out_cards;

        vector<int> curr_deck(DECK);

        float p_win;
        float expected_value;

        std::cout << endl << endl << "+------------------------------+" << endl;
        std::cout <<                 "|           NEW HAND           |" << endl;
        std::cout <<                 "+------------------------------+" << endl << endl;

        std::cout << "Hand # " << hand_count << ":" << endl;

        // get my cards
        while(signal == 1) {
            std::cout << endl << "Your cards: ";
            cin >> in_hand;
            signal = process_input_preflop(in_hand, my_cards, curr_deck);
            if(signal < 0) break;
        }
        if (signal == -2) break;
        if (signal == -1) {
            if (should_record) {

                signal = process_stack_size_input(record_file, stack);
                
                while (signal < 0) {
                    if (signal == -2) break;
                    signal = process_stack_size_input(record_file, stack);
                }
                if (signal == -2) break;
            }
            continue;
        }
        std::cout << "Number of opponents: ";
        cin >> start_num_opponents;
        p_win = pre_flop_win_estimate(my_cards, start_num_opponents);
        std::cout << "P(win): " << p_win * 100 << "%" << endl;

        // get flop
        signal = 1;
        while(signal == 1) {
            std::cout << endl << "Flop: ";
            cin >> in_hand;
            signal = process_input_flop(in_hand, out_cards, curr_deck);
            if(signal < 0) break;
        }
        if (signal == -2) break;
        if (signal == -1) {
            if (should_record) {

                signal = process_stack_size_input(record_file, stack);
                
                while (signal < 0) {
                    if (signal == -2) break;
                    signal = process_stack_size_input(record_file, stack);
                }
                if (signal == -2) break;
            }
            continue;
        }
        std::cout << "Number of opponents: ";
        cin >> num_opponents;
        p_win = win_estimate(my_cards, out_cards, start_num_opponents, num_opponents, NUM_SAMPLES);
        std::cout << "P(win): " << p_win * 100 << "%" << endl;

        // get turn
        signal = 1;
        while(signal == 1) {
            std::cout << endl << "Turn: ";
            cin >> in_hand;
            signal = process_input_turn(in_hand, out_cards, curr_deck);
            if(signal < 0) break;
        }
        if (signal == -2) break;
        if (signal == -1) {
            if (should_record) {

                signal = process_stack_size_input(record_file, stack);
                
                while (signal < 0) {
                    if (signal == -2) break;
                    signal = process_stack_size_input(record_file, stack);
                }
                if (signal == -2) break;
            }
            continue;
        }
        std::cout << "Number of opponents: ";
        cin >> num_opponents;
        p_win = win_estimate(my_cards, out_cards, start_num_opponents, num_opponents, NUM_SAMPLES);
        std::cout << "P(win): " << p_win * 100 << "%" << endl;

        // get river
        signal = 1;
        while(signal == 1) {
            std::cout << endl << "River: ";
            cin >> in_hand;
            signal = process_input_river(in_hand, out_cards, curr_deck);
            if(signal < 0) break;
        }
        if (signal == -2) break;
        if (signal == -1) {
            if (should_record) {

                signal = process_stack_size_input(record_file, stack);
                
                while (signal < 0) {
                    if (signal == -2) break;
                    signal = process_stack_size_input(record_file, stack);
                }
                if (signal == -2) break;
            }
            continue;
        }
        std::cout << "Number of opponents: ";
        cin >> num_opponents;
        p_win = win_estimate(my_cards, out_cards, start_num_opponents, num_opponents, NUM_SAMPLES);
        std::cout << "P(win): " << p_win * 100 << "%" << endl;

        if (should_record) {

            signal = process_stack_size_input(record_file, stack);
                
                while (signal < 0) {
                    if (signal == -2) break;
                    signal = process_stack_size_input(record_file, stack);
                }
                if (signal == -2) break;
        }
    }

    record_file.close();

	return 0;
}