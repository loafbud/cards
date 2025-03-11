#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define DECK_SIZE 52

#define COLOR_RED   0
#define COLOR_BLACK 1
#define COLOR_VOID  2

#define VALUE_ACE   1 
#define VALUE_TWO   2
#define VALUE_THREE 3
#define VALUE_FOUR  4
#define VALUE_FIVE  5 
#define VALUE_SIX   6
#define VALUE_SEVEN 7
#define VALUE_EIGHT 8
#define VALUE_NINE  9 
#define VALUE_TEN   10
#define VALUE_JACK  11
#define VALUE_QUEEN 12 
#define VALUE_KING 13
#define VALUE_VOID 0

#define SUIT_HEARTS 0
#define SUIT_SPADES 1
#define SUIT_DIAMONDS 2
#define SUIT_CLUBS 3

#define RED "\x1B[31m"
#define RESET "\x1B[0m"

// VSF is value suit format ex. QS, 5H)

enum card_value {
    // each number is represented by an integer
    // 0 indicates an invalid or empty card
    ace   = VALUE_ACE,
    two   = VALUE_TWO,
    three = VALUE_THREE,
    four  = VALUE_FOUR,
    five  = VALUE_FIVE,
    six   = VALUE_SIX,
    seven = VALUE_SEVEN,
    eight = VALUE_EIGHT,
    nine  = VALUE_NINE,
    ten   = VALUE_TEN,
    jack  = VALUE_JACK,
    queen = VALUE_QUEEN,
    king  = VALUE_KING
};
enum card_suit {
    // each suit is represented by an integer
    hearts = 0,
    spades = 1,
    diamonds = 2,
    clubs = 3
};

struct Card {
    // the card's defining traits are its value (number) and suit (HSDC)
    enum card_value value;
    enum card_suit suit;
};
struct Deck {
    // a deck is simply an array of 52 cards
    struct Card deck_card[52];
};
void identify_card(struct Card the_card) {
    // prints the card info in VSF
    char card_info[3];
    switch(the_card.value) {
        case ace: 
            card_info[0] = 'A';
            break;
        case two: 
            card_info[0] = '2';
            break;
        case three: 
            card_info[0] = '3';
            break;
        case four: 
            card_info[0] = '4';
            break;
        case five: 
            card_info[0] = '5';
            break;
        case six: 
            card_info[0] = '6';
            break;
        case seven: 
            card_info[0] = '7';
            break;
        case eight:
            card_info[0] = '8';
            break;
        case nine:
            card_info[0] = '9';
            break;
        case ten:
            card_info[0] = 'T';
            break;
        case jack:
            card_info[0] = 'J';
            break;
        case queen:
            card_info[0] = 'Q';
            break;
        case king:
            card_info[0] = 'K';
            break;
        default:
            printf("Error: Card was not identified within Ace-King\nCard read as %u",the_card.value);
            return;
    }
    switch(the_card.suit) {
        case hearts:
            card_info[1] = 'H';
            break;
        case spades:
            card_info[1] = 'S';
            break;
        case diamonds:
            card_info[1] = 'D';
            break;
        case clubs:
            card_info[1] = 'C';
            break;
        default:
            printf("Error: Card was not identified as heart/spade/diamond/club\n\n");
            return;
    }
    if (the_card.suit % 2 == 0) {
        // print red
        printf(RED "%s" RESET,card_info);
    }
    else {
        // print standard
        printf("%s",card_info);
    }
    
    
    return;
}   

void swap_card(struct Deck * the_deck, int card1, int card2) {
    // takes a pointer to the deck, indices.
    // swaps two cards within the deck indexed by card1, card2
    struct Card hold_card;
    hold_card.value = the_deck->deck_card[card1].value;
    hold_card.suit = the_deck->deck_card[card1].suit;

    the_deck->deck_card[card1].value = the_deck->deck_card[card2].value;
    the_deck->deck_card[card1].suit  = the_deck->deck_card[card2].suit;

    the_deck->deck_card[card2].value = hold_card.value;
    the_deck->deck_card[card2].suit  = hold_card.suit;
    
    return;
}
struct Deck * initialize_ordered_deck() {
    struct Deck * ordered_deck = malloc(sizeof(struct Deck));
    for (int i = 0; i < 52; i++) {
        ordered_deck->deck_card[i].value = i % 13 + 1; // A-K
        ordered_deck->deck_card[i].suit = floor(i / 13); // HSDC
    }
    return ordered_deck;
}
void display_deck(struct Deck * the_deck) {
    // prints out all the cards in VSF
    for (int i = 0; i < 52; i++) {
        identify_card(the_deck->deck_card[i]);
        printf("\n");
    }
    return;
}
struct Deck * shuffle_deck(struct Deck * the_deck) {
    for (int shuffles = 0; shuffles < 7; shuffles++) {
        srand(time(NULL));
        for (int i = 0; i < DECK_SIZE; i++) {
            int j = rand() % 52;
            swap_card(the_deck,i,j);
        }
    }
    return the_deck;
}
struct Deck * initialize_shuffled_deck() {
    return shuffle_deck(initialize_ordered_deck());
}
int is_valid_deck(struct Deck * the_deck) {
    int validity = 1;
    if ((int)sizeof(the_deck->deck_card)/(int)sizeof(the_deck->deck_card[0]) != 52) {
        // sizeof returns bytes, so by dividing the size of the 0th element, 
        // it returns number of elements/cards
        printf("NOT A 52 CARD DECK\nDECK HAS %d CARDS\n",(int)sizeof(the_deck->deck_card));
        return 0; // invalid deck, does not have 52 cards
    }
    struct Deck * ordered_deck = initialize_ordered_deck();
    for (int i = 0; i < 52; i++) {
        int instance = 0;
        for (int j = 0; j < 52; j++) {
            if (ordered_deck->deck_card[i].value == the_deck->deck_card[j].value 
            &&  ordered_deck->deck_card[i].suit  == the_deck->deck_card[j].suit ) {
                instance++;
            }
        }
        if (instance != 1) {
            printf("HAS %d INSTANCES OF ",instance);
            identify_card(ordered_deck->deck_card[i]);
            validity = 0; // invalid deck, missing a card OR has more than one of a card
        }
    }
    if (validity == 0) {
        return 0;
    }
    else {
        return 1;
    }
}