#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#define TABLEAU_STACK_QUANTITY 7  // 7 piles
#define TABLEAU_STACK_MAX     19 // at full capacity, 13 cards with 6 hidden cards

#define STOCK_STACK_MAX       24
#define WASTE_STACK_MAX       24

#define FOUNDATIONS_STACK_MAX     13
#define FOUNDATIONS_STACK_QUANTITY 4

#define HIDDEN_CARDS_MAX 21

#define AUTOCOMPLETE_INTERVAL 200 // milliseconds

#define MOVE_DRAW_STOCK             0
#define MOVE_INTRATABULAR           1
#define MOVE_TABLEAU_TO_FOUNDATIONS 2
#define MOVE_WASTE_TO_FOUNDATIONS   3
#define MOVE_WASTE_TO_TABLEAU       4
#define MOVE_PEEL_FOUNDATIONS       5
#define MOVES_MAX                1000

struct Klondike_Draw {
    // stock and waste
    struct Card * stock; // starts out at 24 and only goes down
    struct Card * waste; // pile where the stock cards are drawn and viewed
};
struct Klondike_Foundations{ 
    // ace stacks all contain 0-13 cards
    struct Card * stack[4];
};
struct Tableau_Stack {
    // each stack can contain up to 19 cards
    struct Card stack_card[TABLEAU_STACK_MAX];
    int hidden_cards; // number of cards that should be hidden
};
struct Klondike_Tableau {
    // the main seven stacks
    struct Tableau_Stack tab_stack[TABLEAU_STACK_QUANTITY];
};
struct Klondike_Move {
    int  move_type;

    int cards_moved; // for intratabular this value can be more than 1
    int tableau_stack_from;
    int tableau_stack_index_from;
    int waste_index_from; // card moved from this index of waste
    int draw_stock_index; // card moved from this index of stock


    int waste_index_to;   // card moved to this index of waste
    int tableau_stack_to; 
    int tableau_stack_index_to;
    int foundations_suit;  // card moved to this stack
    int foundations_index; // card moved to this index of foundations

    int revealed; // keeps track of if a tableau hidden card was revealed
    
    // possible moves:
    // draw stock
    // waste to foundations
    // waste to tableau
    // intratabular
    // tableau to foundations
    // peel from foundations
};
struct Klondike_Solitaire {
    struct Klondike_Draw * draw;
    struct Klondike_Foundations * foundations;
    struct Klondike_Tableau * tableau;
    struct Klondike_Move * moves;
    int moves_index;
    int net_moves;
};

void display_klondike_tutorial() {
    printf("\n------------------------------------------------------------------------------------------\n");
    printf("~Welcome to the Klondike Solitaire Simulator~\n\n");
    printf("This message explains how to play Klondike with this interface.\n");
    printf("For klondike rules, type R.\n");
    printf("Please ensure that your terminal is wide enough to fit the interface below.\n");
    printf("XX represents the back of a card. The numbers of the displayed cards are self-explanatory.\n");
    printf("i.e. A = Ace, 2 = 2, T = 10, etc.\n");
    printf("A blank stack is denoted [].\n");
    printf("To draw from the stock, type S.\n");
    printf("To move a card from the waste to the tableau, type W,\n");
    printf("and then select the tableau stack index (0-6) to where you would like to move the card.\n\n");
    printf("To move card from one tableau stack to another, type T, give the stack index (0-6),\n");
    printf("give the location on that stack (0+), and give the stack index of the stack to where you would like to move the card(s).\n\n");
    printf("To move a card from either the waste or the tableau to the foundations, type F.\n");
    printf("The program will prompt you to select from where you would like to pull, and if applicable, which tableau stack.\n\n");
    printf("To peel a card from the foundations, type P, and select where you would like to move it.\n\n");
    printf("To undo, type U. This counts as a move.\n\n");
    printf("Eventually, if you have revealed all hidden cards in the tableau, you may autocomplete by typing A\n\n");
    printf("To exit, press Ctrl+C\n\n");
    printf("Have fun!\n");
    printf("------------------------------------------------------------------------------------------\n\n");
    return;
}
void display_klondike_rules() {
    printf("\nThe rules displayed below are adapted from https://en.wikipedia.org/wiki/Klondike_(solitaire)#Rules\n\n");
    printf("Klondike is played with a standard 52-card deck, without Jokers.\n\n");
    printf("After shuffling, a tableau of seven fanned piles of cards is laid from left to right.\n");
    printf("From left to right, each pile contains one more card than the last.\n");
    printf("The first and left-most pile contains a single upturned card, the second pile contains two cards, and so forth.\n");
    printf("The topmost card of each pile is turned face up.\n");

    printf("The remaining cards form the stock and are placed facedown at the upper left corner.\n");

    printf("The four foundations are built up by suit from Ace (low in this game) to King,\n");
    printf("and the tableau piles can be built up by alternate colors.\n");
    printf("Every face-up card in a partial pile, or a complete pile, can be moved, as a unit, to another tableau pile on the basis of its highest card.\n"); 
    printf("Any empty piles can be filled with a King, or a pile of cards with a King.\n");
    printf("The aim of the game is to build up four stacks of cards starting with Ace and ending with King,\n");
    printf("all of the same suit, on one of the four foundations, at which time the player would have won.\n\n");

    printf("There are a few different ways of dealing the remainder of the deck from the stock to the waste, but in this program:\n\n");
    printf("One card at a time is drawn from the stock to the waste, with no limit on passes through the deck.\n\n");
    printf("If the player can no longer make any meaningful moves, the game is considered lost. At this point, winning is impossible.\n\n");
}
void delay(unsigned int period_in_ms) {
    clock_t ticks;
    clock_t ticks2;
    clock_t clocks_per_ms = CLOCKS_PER_SEC/1000;
    clock_t clocks_per_period = clocks_per_ms * period_in_ms;
    ticks = clock();
    ticks2 = ticks;
    while ( (ticks2 / clocks_per_period) - (ticks / clocks_per_period) < 1 ) {
        ticks2 = clock();
    }
    return;
}
int get_stack_size(struct Card * the_stack) {
    int i = 0;
    int card_count = 0;
    while(the_stack[i].value != 0) {
        card_count++;
        i++;
    }
    return card_count;
}

struct Klondike_Solitaire * init_klondike(struct Deck * deck) {
    struct Klondike_Solitaire * solitaire = malloc(sizeof(struct Klondike_Solitaire));
    // initialize move system
    solitaire->moves = malloc(MOVES_MAX*sizeof(struct Klondike_Move));
    solitaire->moves_index = 0;
    solitaire->net_moves = 0;

    // initialize tableau
    solitaire->tableau = malloc(TABLEAU_STACK_QUANTITY*(TABLEAU_STACK_MAX+1)*sizeof(struct Card)); 
    int i;
    int j;
    int card_index = 0;
    for (i = 0; i < TABLEAU_STACK_QUANTITY; i++) { // bottom to top
        for (j = 0; j < TABLEAU_STACK_QUANTITY; j++) { // left to right
            if (i <= j) {
                solitaire->tableau->tab_stack[j].stack_card[i] = deck->deck_card[card_index++];
                // traditional dealing order
            }
        }
        solitaire->tableau->tab_stack[i].hidden_cards = i; // hide i cards where i is the ith stack (starting with 0)
    }

    // initializes stock and waste stacks
    solitaire->draw = malloc(sizeof(struct Klondike_Draw));
    solitaire->draw->stock = malloc((STOCK_STACK_MAX+1)*sizeof(struct Card)); // malloc an extra card so that the value can be checked for validity
    solitaire->draw->waste = malloc((WASTE_STACK_MAX+1)*sizeof(struct Card));
    int stock_index = 0;
    for(i = DECK_SIZE - STOCK_STACK_MAX; i < DECK_SIZE; i++) {
        solitaire->draw->stock[stock_index++] = deck->deck_card[i];
    }

    // initialize foundations
    solitaire->foundations = malloc(sizeof(struct Klondike_Foundations));

    solitaire->foundations->stack[SUIT_HEARTS  ]    = malloc((FOUNDATIONS_STACK_MAX+1)*sizeof(struct Card));
    solitaire->foundations->stack[SUIT_SPADES  ]    = malloc((FOUNDATIONS_STACK_MAX+1)*sizeof(struct Card));
    solitaire->foundations->stack[SUIT_DIAMONDS]    = malloc((FOUNDATIONS_STACK_MAX+1)*sizeof(struct Card));
    solitaire->foundations->stack[SUIT_CLUBS   ]    = malloc((FOUNDATIONS_STACK_MAX+1)*sizeof(struct Card));

    return solitaire;
}
void unspool_draw(struct Klondike_Draw * the_stock) {
    // for debugging purposes
    int i;
    for (i = 0; i < STOCK_STACK_MAX; i++) {
        if (the_stock->stock[i].value) {
            printf("stock[%d] = ",i);
            identify_card(the_stock->stock[i]);
        }
        else {
            printf("EMPTY STOCK");
        }
        printf("\n");
    }
    printf("\n");
    for(i = 0; i < STOCK_STACK_MAX; i++) {
        if (the_stock->waste[i].value) {
            identify_card(the_stock->waste[i]);
        }
        else {
            printf("EMPTY WASTE");
        }
        printf("\n");
    }
    return;
}
int is_valid_klondike(struct Klondike_Solitaire * solitaire) {
    int i;
    int j;
    struct Deck deck;
    int card_index = 0;
    // collect cards from tableau into the deck
    int tableau_stack_size;
    for (i = 0; i < TABLEAU_STACK_QUANTITY; i++) {
        tableau_stack_size = get_stack_size(solitaire->tableau->tab_stack[i].stack_card);
        for (j = 0; j < tableau_stack_size; j++) {
            deck.deck_card[card_index++] = solitaire->tableau->tab_stack[i].stack_card[j];
        }
    }
    // collect cards from stock
    int stock_stack_size = get_stack_size(solitaire->draw->stock);
    for(i = 0; i < stock_stack_size; i++) {
        deck.deck_card[card_index++] = solitaire->draw->stock[i];
    }
    // collect cards from waste
    int waste_stack_size = get_stack_size(solitaire->draw->waste);
    for(i = 0; i < waste_stack_size; i++) {
        deck.deck_card[card_index++] = solitaire->draw->waste[i];
    }
    // collect cards from foundations
    int foundations_stack_size;
    for (i = 0; i < FOUNDATIONS_STACK_QUANTITY; i++) {
        foundations_stack_size = get_stack_size(solitaire->foundations->stack[i]);
        for (j = 0; j < foundations_stack_size; j++) {
            deck.deck_card[card_index++] = solitaire->foundations->stack[i][j];
        }
    }
    return is_valid_deck(&deck);
}
void display_klondike(struct Klondike_Solitaire * solitaire) {
    // display stock
    printf("\n");
    int stock_stack_size = get_stack_size(solitaire->draw->stock);
    if (stock_stack_size == 0) {
        // stock is empty
        printf(" [] ");
    }
    else {
        printf(" XX ");
    }
    int waste_stack_size = get_stack_size(solitaire->draw->waste);
    int waste_index = waste_stack_size - 1;
    if (waste_stack_size == 0) {
        printf(" [] ");
    }
    else {
        printf(" ");
        identify_card(solitaire->draw->waste[waste_index]);
        printf(" ");
    }
    printf("    ");

    // display foundations
    int i;
    for (i = 0; i < FOUNDATIONS_STACK_QUANTITY; i++) {
        int size = get_stack_size(solitaire->foundations->stack[i]);
        printf(" ");
        if (size == 0) {
            printf("[]");
        }
        else {
        identify_card(solitaire->foundations->stack[i][size - 1]);
        }
        printf(" ");
    }
    // display moves count
    printf("   Moves: %d",solitaire->net_moves);
    printf("\n\n");

    // display tableau
    int stack_size[TABLEAU_STACK_QUANTITY]; // an array of the sizes of the tableau stacks
    int largest_stack_size;
    int hide = 1; // debugging flag for the tableau (set to 1 to see full tableau)
    int j;
    // initial tableau template (displayed cards are index 0)
    //                         C6 
    //                     C5  XX 
    //                 C4  XX  XX 
    //             C3  XX  XX  XX 
    //         C2  XX  XX  XX  XX 
    //     C1  XX  XX  XX  XX  XX 
    // C0  XX  XX  XX  XX  XX  XX
    //
    for(i = 0; i < TABLEAU_STACK_QUANTITY; i++) { // left to right
        // initialize stack_size array
        stack_size[i] = get_stack_size(solitaire->tableau->tab_stack[i].stack_card);
        if (i > 0) {
            if (stack_size[i] > largest_stack_size) largest_stack_size = stack_size[i];
        }
        else {
            largest_stack_size = stack_size[i];
        }
    }
    // actually displaying the tableau
    for(j = largest_stack_size - 1; j >= 0; j--) { // TOP TO BOTTOM
        for(i = 0; i < TABLEAU_STACK_QUANTITY; i++) { // LEFT TO RIGHT
            if (solitaire->tableau->tab_stack[i].stack_card[j].value == 0) {
                if (j == 0) {
                    printf(" [] ");
                }
                else {
                    printf("    "); // no card at this level
                }
            }
            else if ( (j >= solitaire->tableau->tab_stack[i].hidden_cards) || hide == 0 ) { // only display cards above the number of hidden cards
                printf(" ");
                identify_card(solitaire->tableau->tab_stack[i].stack_card[j]);
                printf(" ");
            }
            else {
                printf(" XX ");
            }
        }
        printf("\n");
    }
    printf("\n\n");
    return;
}
void draw_stock(struct Klondike_Solitaire * solitaire) {
    int waste_stack_size = get_stack_size(solitaire->draw->waste); 
    int stock_stack_size = get_stack_size(solitaire->draw->stock);
    int waste_index;
    int stock_index;
    if (waste_stack_size == 0 && stock_stack_size == 0) {
        // stock and waste are empty
        printf("The stock (draw) and waste (discard) stacks are empty.\n");
        return;
    }
    else if (waste_stack_size != 0 && stock_stack_size == 0) {
        // stock is empty, waste is not empty
        // move entire waste stack to stock stack
        int cards_count = waste_stack_size;

        
        waste_index = waste_stack_size - 1;
        for (stock_index = 0; stock_index < cards_count; stock_index++) {
            solitaire->draw->stock[stock_index] = solitaire->draw->waste[waste_index]; // move from waste to stock
            solitaire->draw->waste[waste_index].value = VALUE_VOID; // empty waste card
            waste_index--;
        }
        // update move part 1
        solitaire->moves[solitaire->moves_index].cards_moved = cards_count;
    }
    else {
        waste_index = waste_stack_size; // not -1 because it is receiving cards
        stock_index = stock_stack_size - 1; // -1 because it is sending cards
        // stock is not empty and waste is not empty
        solitaire->draw->waste[waste_index] = solitaire->draw->stock[stock_index];
        // invalidate card
        solitaire->draw->stock[stock_index].value = VALUE_VOID;

        // update move part 1
        solitaire->moves[solitaire->moves_index].cards_moved = 1;
        solitaire->moves[solitaire->moves_index].draw_stock_index = stock_index;
        solitaire->moves[solitaire->moves_index].waste_index_to = waste_index;
    }
    // update move part 2
    solitaire->moves[solitaire->moves_index].move_type = MOVE_DRAW_STOCK;
    solitaire->moves_index++;
    solitaire->net_moves++;
    return;
}

int get_suit(struct Card source_card) {
    if (source_card.suit < 0 || source_card.suit > 3) {
        // ERROR
        // card is undefined
        return -1;
    }
    else {
        return source_card.suit;
    }
}
int get_color(struct Card source_card) {
    // returns color of source_card
    if (source_card.suit < 0 || source_card.suit > 3) {
        // ERROR
        // card is undefined
        return -1;
    }
    else {
        return source_card.suit % 2;
    }
}
int get_value(struct Card source_card) {
    // returns value of source_card
    // recall that  void is 0
    //              ace is 1 ... 
    //              king is 13
    if (source_card.value < VALUE_ACE || source_card.value > VALUE_KING) {
        // ERROR
        // card is undefined
        return -1;
    }
    else {
        return source_card.value;
    }

}
void flip_revealed_card(struct Klondike_Solitaire * solitaire, int source_stack) {
    // check to see if the top card from the tableau stack is hidden.
    int source_stack_size = get_stack_size(solitaire->tableau->tab_stack[source_stack].stack_card);
    if (source_stack_size == solitaire->tableau->tab_stack[source_stack].hidden_cards && source_stack_size != 0) {
        // reveal it
        solitaire->tableau->tab_stack[source_stack].hidden_cards--;
        solitaire->moves[solitaire->moves_index].revealed++;
    }
    return;
}
void intratabular_move(struct Klondike_Solitaire * solitaire, int source_stack, int source_position, int dest_stack, int override) {
    // intra = within/inside
    // tabular = tableau
    int legal;
    int accessible;
    // (void)dest_stack;
    // get suit/color and value of top card of destination stack
    int source_stack_size = get_stack_size(solitaire->tableau->tab_stack[source_stack].stack_card);
    int dest_stack_size = get_stack_size(solitaire->tableau->tab_stack[dest_stack].stack_card);
    int source_color = get_color(solitaire->tableau->tab_stack[source_stack].stack_card[source_position]);
    int source_value = get_value(solitaire->tableau->tab_stack[source_stack].stack_card[source_position]);
    int dest_color;
    int dest_value;
    
    if (source_color == -1) {
        printf("Error 334: source color not recognized\n");
        return;
    }
    if (source_value == -1) {
        printf("Error 338: source value not recognized\n");
        return;
    }

    if (dest_stack_size == 0) {
        // stack is empty
        dest_color = COLOR_VOID;
        dest_value = VALUE_VOID;
    }
    else {
        dest_color = get_color(solitaire->tableau->tab_stack[dest_stack].stack_card[dest_stack_size-1]);
        dest_value = get_value(solitaire->tableau->tab_stack[dest_stack].stack_card[dest_stack_size-1]);
        if (dest_color != COLOR_RED && dest_color != COLOR_BLACK && dest_color != COLOR_VOID) {
            printf("Error 351: destination color not recognized\n");
            return;
        }
    }
    legal = ((dest_value == VALUE_VOID) && (source_value == VALUE_KING)) /* moving a king to an empty stack */ || 
            ((dest_color != source_color) && (dest_value == source_value + 1)) /* moving a card  from one stack to another non-empty stack */  
            ? 1 // if true, move is legal
            : 0;
    if (!legal && !override) {
        printf("Error 360: Illegal move\n");
        // debugging
        // printf("dest_color = %d\ndest_value = %d\nsource_color = %d\nsource_value = %d\n",dest_color,dest_value,source_color,source_value);
        return;
    }
    else {
        int i;
        // cards are compatible
        //
        // check if the card is accessible (part of a legal stack and not hidden)
        if ( (source_position == source_stack_size - 1) && (solitaire->tableau->tab_stack[source_stack].hidden_cards <= source_position) ) {
            accessible = 1;
        }
        else {
            int cards_away = 1; // 
            for (i = source_position + 1; i < source_stack_size; i++) {
                if (get_value(solitaire->tableau->tab_stack[source_stack].stack_card[i]) != source_value - cards_away) {
                    accessible = 0;
                }
                cards_away++;
            }
        }
        if (accessible == 0 && override == 0) {
            printf("Error 367: Card is not accessible\n");
            return;
        }
        int cards_moved = 0;

        // update move part 1
        solitaire->moves[solitaire->moves_index].tableau_stack_from = source_stack;
        solitaire->moves[solitaire->moves_index].tableau_stack_index_from = source_position;
        solitaire->moves[solitaire->moves_index].tableau_stack_to = dest_stack;
        solitaire->moves[solitaire->moves_index].tableau_stack_index_to = dest_stack_size;

        for (i = source_position; i < source_stack_size; i++) {
            solitaire->tableau->tab_stack[dest_stack].stack_card[dest_stack_size] = solitaire->tableau->tab_stack[source_stack].stack_card[i];
            dest_stack_size++;
            cards_moved++;
        }
        for(i = source_stack_size-1; i > source_stack_size - cards_moved - 1; i--) {
            // invalidate moved cards
            solitaire->tableau->tab_stack[source_stack].stack_card[i].value = VALUE_VOID;
        }
        // update move part 2
        solitaire->moves[solitaire->moves_index].cards_moved = cards_moved;
        solitaire->moves[solitaire->moves_index].move_type = MOVE_INTRATABULAR;
        flip_revealed_card(solitaire,source_stack);
        solitaire->moves_index++;
        solitaire->net_moves++;
        return;
        // move the source card (and all cards above it) to top of destination stack
    }
}
void waste_to_tableau_move(struct Klondike_Solitaire * solitaire, int dest_stack) {
    int legal;
    int waste_index = get_stack_size(solitaire->draw->waste) - 1;
    int dest_stack_size = get_stack_size(solitaire->tableau->tab_stack[dest_stack].stack_card);
    int dest_index = dest_stack_size;
    int source_value = get_value(solitaire->draw->waste[waste_index]);
    int value_below = get_value(solitaire->tableau->tab_stack[dest_stack].stack_card[dest_index - 1]);
    if (source_value == -1) {
        printf("Error 413: source value not recognized\n");
        return;
    }
    if (waste_index == -1) {
        legal = 0;
        printf("Waste is empty. Move rejected\n");
        return;
    }
    else if (dest_stack_size == 0 && source_value == VALUE_KING) {
        // destination stack is empty so it's legal
        legal = 1;
    }
    else {
        // cards exist. check for move legality
        int source_color = get_color(solitaire->draw->waste[waste_index]);
        int dest_color = get_color(solitaire->tableau->tab_stack[dest_stack].stack_card[dest_index - 1]); 
        if (source_color == -1) {
            printf("Error 429: source color not recognized\n");
            return;
        }
        if ( (source_color != dest_color) && (value_below == source_value + 1) ) {
            legal = 1;
        }
        else {
            legal = 0;
            printf("Cards not compatible or card does not exist. Move rejected\n");
            return;
        }
    }
    if (legal) {
        // execute the move
        solitaire->tableau->tab_stack[dest_stack].stack_card[dest_index] = solitaire->draw->waste[waste_index];
        solitaire->draw->waste[waste_index].value = VALUE_VOID;
        // update move
        solitaire->moves[solitaire->moves_index].waste_index_from = waste_index;
        solitaire->moves[solitaire->moves_index].tableau_stack_to = dest_stack;
        solitaire->moves[solitaire->moves_index].tableau_stack_index_to = dest_index;
        solitaire->moves[solitaire->moves_index].move_type = MOVE_WASTE_TO_TABLEAU;
        solitaire->moves_index++;
        solitaire->net_moves++;
    }
    return;
}
void to_foundations_move(struct Klondike_Solitaire * solitaire, int source_stack, int from_waste) {
    // if moving from tableau, set from_waste to 0,
    // if moving from waste, set from_waste to 1 (source_stack can be anything, but just set it to 0)

    // this function is general and works for stock/waste and tableau. 
    // However, flip_revealed_card() must be used after a tableau to foundations move.
    // source_stack = which tableau stack to move from
    // source_index = which card to move

    int source_index;
    int source_suit;
    int source_value;
    if (from_waste) {
        source_index = get_stack_size(solitaire->draw->waste) - 1;
        source_suit  = get_suit(solitaire->draw->waste[source_index]);
        source_value = get_value(solitaire->draw->waste[source_index]);
    }
    else {
        source_index = get_stack_size(solitaire->tableau->tab_stack[source_stack].stack_card) - 1;
        source_suit  = get_suit(solitaire->tableau->tab_stack[source_stack].stack_card[source_index]);
        source_value = get_value(solitaire->tableau->tab_stack[source_stack].stack_card[source_index]);
    }
    int dest_index   = get_stack_size(solitaire->foundations->stack[source_suit]);
    int dest_value   = get_value(solitaire->foundations->stack[source_suit][dest_index - 1]);
    if (source_index != -1 && ((dest_index == 0 && source_value == VALUE_ACE) || (dest_value == source_value - 1))) {
        // empty foundations pile and card being moved is an ace
        // OR
        // foundations card is the value of the card being moved minus one
        // e.g. 4H will be played over a 3H

        // move card (ace) from tableau stack to the corresponding foundations
        solitaire->moves[solitaire->moves_index].foundations_suit = source_suit;
        solitaire->moves[solitaire->moves_index].foundations_index = dest_index;
        if (from_waste) {
            solitaire->foundations->stack[source_suit][dest_index] = solitaire->draw->waste[source_index];
            solitaire->draw->waste[source_index].value = VALUE_VOID;
            // move update
            solitaire->moves[solitaire->moves_index].waste_index_from = source_index;
            solitaire->moves[solitaire->moves_index].move_type = MOVE_WASTE_TO_FOUNDATIONS;
        }
        else {
            solitaire->foundations->stack[source_suit][dest_index] = solitaire->tableau->tab_stack[source_stack].stack_card[source_index];
            solitaire->tableau->tab_stack[source_stack].stack_card[source_index].value = VALUE_VOID;
            // move update
            solitaire->moves[solitaire->moves_index].tableau_stack_index_from = source_index;
            solitaire->moves[solitaire->moves_index].tableau_stack_from = source_stack;
            solitaire->moves[solitaire->moves_index].move_type = MOVE_TABLEAU_TO_FOUNDATIONS;
            flip_revealed_card(solitaire,source_stack);
        }
        solitaire->moves_index++;
        solitaire->net_moves++;
        return;
    }
    else {
        printf("Card either does not exist or is not ready to be moved to foundations. Move rejected.\n");
        return;
    }
}
void from_foundations_move(struct Klondike_Solitaire * solitaire, int suit, int dest_stack, int override) {

    int dest_stack_index = get_stack_size(solitaire->tableau->tab_stack[dest_stack].stack_card);
    int foundations_stack_index = get_stack_size(solitaire->foundations->stack[suit]) - 1;
    if (foundations_stack_index == -1) {
        printf("No cards on the selected foundation stack. Move rejected\n");
    }
    else if (override || 
            (dest_stack_index == 0 && solitaire->foundations->stack[suit][foundations_stack_index].value == VALUE_KING) || 
            (
                (solitaire->tableau->tab_stack[dest_stack].stack_card[dest_stack_index-1].value == solitaire->foundations->stack[suit][foundations_stack_index].value + 1) &&
                ( get_color(solitaire->tableau->tab_stack[dest_stack].stack_card[dest_stack_index-1]) != suit % 2 )
            )
            )
        {
        // if (tableau stack is empty and the card is a king) OR
        // ( (the value at the top of the tableau stack is equal to the value at the top of the foundations stack + 1) AND 
        // ( their colors are opposites) )
        solitaire->tableau->tab_stack[dest_stack].stack_card[dest_stack_index] = solitaire->foundations->stack[suit][foundations_stack_index];
        solitaire->foundations->stack[suit][foundations_stack_index].value = 0;
    }
    else {
        printf("Cards not compatible. Move rejected.\n");
        return;
    }

    // update move
    solitaire->moves[solitaire->moves_index].move_type = MOVE_PEEL_FOUNDATIONS;
    solitaire->moves[solitaire->moves_index].foundations_suit = suit;
    solitaire->moves[solitaire->moves_index].foundations_index = foundations_stack_index;
    solitaire->moves[solitaire->moves_index].tableau_stack_to = dest_stack;
    solitaire->moves[solitaire->moves_index].tableau_stack_index_to = dest_stack_index;
    solitaire->moves_index++;
    solitaire->net_moves++;

    return;
}
void autocomplete(struct Klondike_Solitaire * solitaire) {
    int i;
    int tab_stack_size[TABLEAU_STACK_QUANTITY];
    int foundations_stack_size[FOUNDATIONS_STACK_QUANTITY];
    int lowest_foundations;
    int desired_suit;
    int desired_value;
    int skip;
    int done = 0;
    while (!done) {
        skip = 0;
        for (i = 0; i < FOUNDATIONS_STACK_QUANTITY; i++) {
            foundations_stack_size[i] = get_stack_size(solitaire->foundations->stack[i]);
            if (i == 0) {
                lowest_foundations = foundations_stack_size[i];
                desired_suit = i;
            }
            else if (foundations_stack_size[i] < lowest_foundations) {
                lowest_foundations = foundations_stack_size[i];
                desired_suit = i;
            }
        }
        desired_value = lowest_foundations + 1;
        if (lowest_foundations == VALUE_KING) {
            done = 1;
            break;
        }
        for (i = 0; i < TABLEAU_STACK_QUANTITY; i++) {
            tab_stack_size[i] = get_stack_size(solitaire->tableau->tab_stack[i].stack_card);
            if (tab_stack_size[i] == 0) {
                // don't bother looking at empty stacks
                continue;
            }
            if (get_value(solitaire->tableau->tab_stack[i].stack_card[tab_stack_size[i] - 1]) == desired_value &&
                 get_suit(solitaire->tableau->tab_stack[i].stack_card[tab_stack_size[i] - 1])  == desired_suit ) {
                    // desired card located in foundations
                    to_foundations_move(solitaire,i,0);
                    skip = 1;
                    break;
                }
        }
        while (!skip) {
            int stock_stack_size = get_stack_size(solitaire->draw->stock);
            int waste_stack_size = get_stack_size(solitaire->draw->waste);
            // card was not in tableau, check stack/waste
            if (waste_stack_size > 0 && 
                get_value(solitaire->draw->waste[waste_stack_size - 1]) == desired_value && 
                 get_suit(solitaire->draw->waste[waste_stack_size - 1])  == desired_suit ) {
                // waste has at least a card and
                // desired card located in waste
                to_foundations_move(solitaire,0,1);
                skip = 1;
            }
            else if (stock_stack_size + waste_stack_size > 0) {
                // draw stock and try again
                draw_stock(solitaire);
            }
            else {
                // stock and waste are empty
                printf("Autocomplete failed. Next foundations card not located in tableau or stock.\n");
                return;
            }
        }
        // post-move
        delay(AUTOCOMPLETE_INTERVAL);
        display_klondike(solitaire);
    }   
    return;
}   
void undo(struct Klondike_Solitaire * solitaire) {
    solitaire->moves_index--;
    int move = solitaire->moves[solitaire->moves_index].move_type;
    switch(move) {
        case MOVE_DRAW_STOCK: {
            if (solitaire->moves[solitaire->moves_index].cards_moved > 1) {
                // refresh waste
                int cards_count = solitaire->moves[solitaire->moves_index].cards_moved;
                int stock_index = cards_count - 1;
                int waste_index;
                for (waste_index = 0; waste_index < cards_count; waste_index++) {
                    solitaire->draw->waste[waste_index] = solitaire->draw->stock[stock_index]; // move from waste to stock
                    solitaire->draw->stock[stock_index].value = VALUE_VOID; // empty waste card
                    stock_index--;
                }
            }
            else {
                // move card backwards from waste to stock
                int stock_index = solitaire->moves[solitaire->moves_index].draw_stock_index;
                int waste_index = solitaire->moves[solitaire->moves_index].waste_index_to;
                solitaire->draw->stock[stock_index] = solitaire->draw->waste[waste_index];
                solitaire->draw->waste[waste_index].value = VALUE_VOID;
            }
            solitaire->net_moves++;
            break;
        }
        case MOVE_INTRATABULAR: {
            // source/dest in this context, not its original context
            int source_stack = solitaire->moves[solitaire->moves_index].tableau_stack_to;
            int source_position = solitaire->moves[solitaire->moves_index].tableau_stack_index_to;
            int dest_stack = solitaire->moves[solitaire->moves_index].tableau_stack_from;
            if (solitaire->moves[solitaire->moves_index].revealed) {
                solitaire->tableau->tab_stack[dest_stack].hidden_cards++;
            }
            intratabular_move(solitaire,source_stack,source_position,dest_stack,1); // override legality for undo
            solitaire->moves_index--;
            break;
        }
        case MOVE_WASTE_TO_TABLEAU: {
            // move the tableau card back to the waste
            int source_stack = solitaire->moves[solitaire->moves_index].tableau_stack_to;
            int source_position = solitaire->moves[solitaire->moves_index].tableau_stack_index_to;
            int waste_index = solitaire->moves[solitaire->moves_index].waste_index_from;
            // execute the move
            solitaire->draw->waste[waste_index] = solitaire->tableau->tab_stack[source_stack].stack_card[source_position];
            solitaire->tableau->tab_stack[source_stack].stack_card[source_position].value = VALUE_VOID;
            solitaire->net_moves++;
            break;
        }
        case MOVE_TABLEAU_TO_FOUNDATIONS: {
            // peel from foundations
            int suit = solitaire->moves[solitaire->moves_index].foundations_suit;
            int dest_stack = solitaire->moves[solitaire->moves_index].tableau_stack_from;
            
            from_foundations_move(solitaire,suit,dest_stack,1); // override legality for undo
            solitaire->moves_index--;
            if (solitaire->moves[solitaire->moves_index].revealed) {
                solitaire->tableau->tab_stack[dest_stack].hidden_cards++;
            }
            break;
        }
        case MOVE_WASTE_TO_FOUNDATIONS: {
            // move foundations card back to the waste
            int source_position = solitaire->moves[solitaire->moves_index].foundations_index;
            int suit = solitaire->moves[solitaire->moves_index].foundations_suit;
            int waste_index = solitaire->moves[solitaire->moves_index].waste_index_from;
            // execute the move
            solitaire->draw->waste[waste_index] = solitaire->foundations->stack[suit][source_position];
            solitaire->foundations->stack[suit][source_position].value = VALUE_VOID;
            solitaire->net_moves++;
            break;
        }
        case MOVE_PEEL_FOUNDATIONS: {
            // move card back to the foundations 
            int source_stack = solitaire->moves[solitaire->moves_index].tableau_stack_to;
            to_foundations_move(solitaire,source_stack,0);
            solitaire->moves_index--;
            break;
        }
        default: {
            printf("Previous move not recognized.\n");
            break;
        }
    }
}
void klondike(struct Deck * the_deck) {
    // initialize all stacks
    display_klondike_tutorial();
    int victory = 0;
    struct Klondike_Solitaire * solitaire;
    solitaire = init_klondike(the_deck);
    int hidden = HIDDEN_CARDS_MAX;
    int revealed = 0;
    while (!victory) {
        if (solitaire->moves_index == MOVES_MAX) {
            printf("Maximum number of moves reached. Please restart.\n");
            return;
        }
        if (is_valid_klondike(solitaire) == 0) {
            printf("Error 797: Klondike deck is faulty.\n");
        }
        display_klondike(solitaire);
        int source_stack;
        int source_position;
        int dest_stack;
        char move;
        int tab_stack_index;
        char wort;
        int peel_foundation;
        int peel_dest;
        if (hidden == 0) {
            printf("You may autocomplete (A).\nMove (S, W, T, F, P, U, A): ");
        }
        else if (solitaire->moves_index == 0) {
            printf("Move (S, W, T, F, P): ");
        }
        else {
            printf("Move (S, W, T, F, P, U): ");
        }
        scanf("%s",&move);
        switch(move) {
            case 'S':
            case 's':
                draw_stock(solitaire);
                break;
            case 'W':
            case 'w':
                // take from waste
                printf("\nTableau stack index: ");
                scanf("%d",&tab_stack_index);
                printf("\n");
                waste_to_tableau_move(solitaire,tab_stack_index);
                break;
            case 'T':
            case 't':
                printf("\n Tab stack index (source): ");
                scanf("%d",&source_stack);
                printf("\n Tab stack position: ");
                scanf("%d",&source_position);
                printf("\n Tab stack index (destination): ");
                scanf("%d",&dest_stack);
                printf("\n");
                if (source_stack < 0 || source_stack > 6 || source_position > TABLEAU_STACK_MAX - 1 || dest_stack < 0 || dest_stack > 6) {
                    printf("Integers were out of bounds. Please try again.\n Note that the tableau stacks are indexed 0-6, and the position must be 0-19\n");
                }
                else {
                    revealed = solitaire->tableau->tab_stack[source_stack].hidden_cards;
                    intratabular_move(solitaire,source_stack,source_position,dest_stack,0);
                    revealed -= solitaire->tableau->tab_stack[source_stack].hidden_cards;
                    
                    hidden -= revealed;
                }
                break;
            case 'F':
            case 'f':
                // foundations
                printf("\nWaste or tableau? (W or T): ");
                scanf("%s",&wort);
                if (wort == 'w' || wort == 'W') {
                    to_foundations_move(solitaire,0,1);
                }
                else if (wort == 't' || wort == 'T') {
                    printf("\nTableau stack index: ");
                    scanf("%d",&tab_stack_index);
                    printf("\n");
                    if (tab_stack_index < 0 || tab_stack_index > 6) {
                        printf("Tableau stack index must be between 0 and 6.\n Please try again.\n");
                    }
                    else {
                        revealed = solitaire->tableau->tab_stack[tab_stack_index].hidden_cards;
                        to_foundations_move(solitaire,tab_stack_index,0);
                        flip_revealed_card(solitaire,tab_stack_index);
                        revealed -= solitaire->tableau->tab_stack[tab_stack_index].hidden_cards;
                        hidden -= revealed;
                    }
                }
                break;
            case 'P':
            case 'p':
                printf("From which foundations pile would you like to peel? ");
                scanf("%d",&peel_foundation);
                printf("To which tableau stack? ");
                scanf("%d", &peel_dest);
                from_foundations_move(solitaire,peel_foundation,peel_dest,0);
                break;
            case 'A':
            case 'a':
                if (hidden == 0) {
                    autocomplete(solitaire);
                }
                else {
                    printf("Autocomplete not available until there are no more hidden cards (XX).\n");
                }
                break;
            case 'U':
            case 'u':
                if (solitaire->moves_index > 0) {
                    undo(solitaire);
                }
                break;
            case 'R':
            case 'r':
                display_klondike_rules();
                break;
            default:
                printf("Move not recognized. Try again\n");
                break;
        }
        if (solitaire->foundations->stack[SUIT_HEARTS  ][FOUNDATIONS_STACK_MAX-1].value == VALUE_KING &&
            solitaire->foundations->stack[SUIT_SPADES  ][FOUNDATIONS_STACK_MAX-1].value == VALUE_KING &&
            solitaire->foundations->stack[SUIT_DIAMONDS][FOUNDATIONS_STACK_MAX-1].value == VALUE_KING &&
            solitaire->foundations->stack[SUIT_CLUBS   ][FOUNDATIONS_STACK_MAX-1].value == VALUE_KING   ) {
                victory = 1;
        }
    }
    if (victory) {
        // victory sequence
        printf(RED "             KH  " RESET "KS  " RED "KD  " RESET "KC\n\n\nYOU WIN\n\nNICE\n\n\n");
    }
    return;
}

// NEXT STEPS:
// GUI
// freecell