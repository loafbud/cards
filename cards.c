#include <stdio.h>
#include <stdlib.h>
#include "cards.h"
#include "klondike.h"

int main() {
    //struct Deck * ordered = initialize_ordered_deck();
    //display_deck(shuffled);
    struct Deck * shuffled = initialize_shuffled_deck();
    klondike(shuffled);
    printf("\n");
    return 1;



}