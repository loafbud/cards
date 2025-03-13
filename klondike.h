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

struct Klondike_Stock {
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
struct Klondike_Solitaire {
    struct Klondike_Stock * draw;
    struct Klondike_Foundations * foundations;
    struct Klondike_Tableau * tableau;
};

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

    // initializes stock and waste stacks
    solitaire->draw = malloc(sizeof(struct Klondike_Stock));
    solitaire->draw->stock = malloc(STOCK_STACK_MAX*sizeof(struct Card));
    solitaire->draw->waste = malloc(WASTE_STACK_MAX*sizeof(struct Card));
    int i;
    int stock_index = 0;
    for(i = DECK_SIZE - STOCK_STACK_MAX; i < DECK_SIZE; i++) {
        solitaire->draw->stock[stock_index++] = deck->deck_card[i];
    }

    // initialize foundations
    solitaire->foundations = malloc(sizeof(struct Klondike_Foundations));

    solitaire->foundations->stack[SUIT_HEARTS  ]    = malloc(FOUNDATIONS_STACK_MAX*sizeof(struct Card));
    solitaire->foundations->stack[SUIT_SPADES  ]    = malloc(FOUNDATIONS_STACK_MAX*sizeof(struct Card));
    solitaire->foundations->stack[SUIT_DIAMONDS]    = malloc(FOUNDATIONS_STACK_MAX*sizeof(struct Card));
    solitaire->foundations->stack[SUIT_CLUBS   ]    = malloc(FOUNDATIONS_STACK_MAX*sizeof(struct Card));

    // initialize tableau
    solitaire->tableau = malloc(TABLEAU_STACK_QUANTITY*(TABLEAU_STACK_MAX)*sizeof(struct Card));
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
    return solitaire;
}
void unspool_stock(struct Klondike_Stock * the_stock) {
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
        printf("    ");
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
                printf("    "); // no card at this level
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
void draw_stock(struct Klondike_Stock * the_stock) {
    int waste_stack_size = get_stack_size(the_stock->waste); 
    int stock_stack_size = get_stack_size(the_stock->stock);
    int waste_index;
    int stock_index;
    if (waste_stack_size == 0 && stock_stack_size == 0) {
        // stock and waste are empty
        printf("The stock (draw) and waste (discard) stacks are empty.\n");
    }
    else if (waste_stack_size != 0 && stock_stack_size == 0) {
        // stock is empty, waste is not empty
        // move entire waste stack to stock stack
        int cards_count = waste_stack_size;
        waste_index = waste_stack_size - 1;
        for (stock_index = 0; stock_index < cards_count; stock_index++) {
            the_stock->stock[stock_index] = the_stock->waste[waste_index]; // move from waste to stock
            the_stock->waste[waste_index].value = 0; // empty waste card
            waste_index--;
        }
    }
    else {
        waste_index = waste_stack_size; // not -1 because it is receiving cards
        stock_index = stock_stack_size - 1; // -1 because it is sending cards
        // stock is not empty and waste is not empty
        the_stock->waste[waste_index] = the_stock->stock[stock_index];
        // invalidate card
        the_stock->stock[stock_index].value = 0;
    }
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
void flip_revealed_card(struct Tableau_Stack * source_stack) {
    // check to see if the top card from the tableau stack is hidden.
    int source_stack_size = get_stack_size(source_stack->stack_card);
    if (source_stack_size == source_stack->hidden_cards && source_stack_size != 0) {
        // reveal it
        source_stack->hidden_cards--;
    }
    return;
}
void intratabular_move(struct Tableau_Stack * source_stack, int source_position, struct Tableau_Stack * destination_stack) {
    // intra = within/inside
    // tabular = tableau
    struct Card * source_card = &source_stack->stack_card[source_position];
    int legal;
    int accessible;
    // (void)destination_stack;
    // get suit/color and value of top card of destination stack
    int source_stack_size = get_stack_size(source_stack->stack_card);
    int dest_stack_size = get_stack_size(destination_stack->stack_card);
    struct Card * dest_card = &destination_stack->stack_card[dest_stack_size-1];
    int source_color = get_color(*source_card);
    int source_value = get_value(*source_card);
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
        dest_color = get_color(*dest_card);
        dest_value = get_value(*dest_card);
        if (dest_color != COLOR_RED && dest_color != COLOR_BLACK && dest_color != COLOR_VOID) {
            printf("Error 351: destination color not recognized\n");
            return;
        }
    }
    legal = ((dest_value == VALUE_VOID) && (source_value == VALUE_KING)) /* moving a king to an empty stack */ || 
            ((dest_color != source_color) && (dest_value == source_value + 1)) /* moving a card  from one stack to another non-empty stack */  
            ? 1 // if true, move is legal
            : 0;
    if (!legal) {
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
        if ( (source_position == source_stack_size - 1) && (source_stack->hidden_cards <= source_position) ) {
            accessible = 1;
        }
        else {
            int cards_away = 1; // 
            for (i = source_position + 1; i < source_stack_size; i++) {
                if ((int)source_stack->stack_card[i].value != source_value - cards_away) {
                    accessible = 0;
                }
                cards_away++;
            }
        }
        if (accessible == 0) {
            printf("Error 367: Card is not accessible\n");
            return;
        }
        int cards_moved = 0;
        for (i = source_position; i < source_stack_size; i++) {
            destination_stack->stack_card[dest_stack_size] = source_stack->stack_card[i];
            dest_stack_size++;
            cards_moved++;
        }
        for(i = source_stack_size-1; i > source_stack_size - cards_moved - 1; i--) {
            // invalidate moved cards
            source_stack->stack_card[i].value = 0;
        }
        flip_revealed_card(source_stack);
        return;
        // move the source card (and all cards above it) to top of destination stack
    }
}
void waste_to_tableau_move(struct Klondike_Stock * the_stock, struct Tableau_Stack * destination_stack) {
    int legal;
    int waste_index = get_stack_size(the_stock->waste) - 1;
    int dest_stack_size = get_stack_size(destination_stack->stack_card);
    int dest_index = dest_stack_size;
    int source_value = get_value(the_stock->waste[waste_index]);
    int value_below = get_value(destination_stack->stack_card[dest_index - 1]);
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
        int source_color = get_color(the_stock->waste[waste_index]);
        int dest_color = get_color(destination_stack->stack_card[dest_index - 1]); 
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
        destination_stack->stack_card[dest_index] = the_stock->waste[waste_index];
        the_stock->waste[waste_index].value = 0;
    }
    return;
}
void to_foundations_move(struct Card * source_stack, struct Klondike_Foundations * foundations) {
    // this function is general and works for stock/waste and tableau. 
    // However, flip_revealed_card() must be used after a tableau to foundations move.
    int source_index = get_stack_size(source_stack) - 1;
    int source_suit  = get_suit(source_stack[source_index]);
    int source_value = get_value(source_stack[source_index]);
    int dest_index   = get_stack_size(foundations->stack[source_suit]);
    int dest_value   = get_value(foundations->stack[source_suit][dest_index - 1]);
    if (source_index != -1 && ((dest_index == 0 && source_value == VALUE_ACE) || (dest_value == source_value - 1))) {
        // empty foundations pile and card being moved is an ace
        // OR
        // foundations card is the value of the card being moved minus one
        // e.g. 4H will be played over a 3H

        // move card (ace) from tableau stack to the corresponding foundations

        // MAKE A FUNCTION OUT OF THIS
        foundations->stack[source_suit][dest_index] = source_stack[source_index];
        source_stack[source_index].value = 0;

        return;
        }
    else {
        printf("Card either does not exist or is not ready to be moved to foundations. Move rejected.\n");
        return;
    }
}
void from_foundations_move(struct Klondike_Foundations * the_foundations, int suit, struct Tableau_Stack * tab_stack) {
    int dest_stack_index = get_stack_size(tab_stack->stack_card);
    int foundations_stack_index = get_stack_size(the_foundations->stack[suit]) - 1;

    if ((dest_stack_index == 0 && the_foundations->stack[suit][foundations_stack_index].value == VALUE_KING) || (
        (tab_stack->stack_card[dest_stack_index-1].value == the_foundations->stack[suit][foundations_stack_index].value + 1) &&
        ( get_color(tab_stack->stack_card[dest_stack_index-1]) != suit % 2 ) )
    ) {
        // if (tableau stack is empty and the card is a king) OR
        // ( (the value at the top of the tableau stack is equal to the value at the top of the foundations stack + 1) AND 
        // ( their colors are opposites) )
        tab_stack->stack_card[dest_stack_index] = the_foundations->stack[suit][foundations_stack_index];
        the_foundations->stack[suit][foundations_stack_index].value = 0;
        return;
    }
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
                    to_foundations_move(solitaire->tableau->tab_stack[i].stack_card,solitaire->foundations);
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
                to_foundations_move(solitaire->draw->waste,solitaire->foundations);
                skip = 1;
            }
            else if (stock_stack_size + waste_stack_size > 0) {
                // draw stock and try again
                draw_stock(solitaire->draw);
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
int klondike(struct Deck * the_deck) {
    // initialize all stacks
    int victory = 0;
    struct Klondike_Solitaire * solitaire;
    solitaire = init_klondike(the_deck);
    int hidden = HIDDEN_CARDS_MAX;
    int revealed = 0;
    while (!victory) {
        display_klondike(solitaire);
        int source_stack;
        int source_position;
        int destination_stack;
        char move;
        int tab_stack_index;
        char wort;
        int peel_foundation;
        int peel_dest;
        if (hidden == 0) {
            printf("You may autocomplete (A).\nMove (S, W, T, F, P, A): ");
        }
        else {
            printf("Move (S, W, T, F, P): ");
        }
        scanf("%s",&move);
        switch(move) {
            case 'S':
            case 's':
                draw_stock(solitaire->draw);
                break;
            case 'W':
            case 'w':
                // take from waste
                printf("\nTableau stack index: ");
                scanf("%d",&tab_stack_index);
                printf("\n");
                waste_to_tableau_move(solitaire->draw,&solitaire->tableau->tab_stack[tab_stack_index]);
                break;
            case 'T':
            case 't':
                printf("\n Tab stack index (source): ");
                scanf("%d",&source_stack);
                printf("\n Tab stack position: ");
                scanf("%d",&source_position);
                printf("\n Tab stack index (destination): ");
                scanf("%d",&destination_stack);
                printf("\n");
                if (source_stack < 0 || source_stack > 6 || source_position > TABLEAU_STACK_MAX - 1 || destination_stack < 0 || destination_stack > 6) {
                    printf("Integers were out of bounds. Please try again.\n Note that the tableau stacks are indexed 0-6, and the position must be 0-19\n");
                }
                else {
                    revealed = solitaire->tableau->tab_stack[source_stack].hidden_cards;
                    intratabular_move(&solitaire->tableau->tab_stack[source_stack],source_position,&solitaire->tableau->tab_stack[destination_stack]);
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
                    to_foundations_move(&solitaire->draw->waste[get_stack_size(solitaire->draw->waste)-1],solitaire->foundations);
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
                        to_foundations_move(solitaire->tableau->tab_stack[tab_stack_index].stack_card,solitaire->foundations);
                        flip_revealed_card(&solitaire->tableau->tab_stack[tab_stack_index]);
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
                from_foundations_move(solitaire->foundations,peel_foundation,&solitaire->tableau->tab_stack[peel_dest]);
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
        printf(RED "             KH  " RESET "KS  " RED "KD  " RESET "KC\n\n\nYOU WIN\n\nNICE\n\n\n");
    }
    // victory sequence
    return 0;
}

// NEXT STEPS:
// undo
// GUI
// freecell