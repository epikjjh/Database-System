#include "bpt.h"

// MAIN

int main( int argc, char ** argv ) {

    char * input_file;
    char value[120];
    FILE * fp;
    node * root;
    int range2;
    int64_t input;
    char instruction;
    char license_part;

    root = NULL;
    verbose_output = false;

    if (argc > 1) {
        order = atoi(argv[1]);
        if (order < MIN_ORDER || order > MAX_ORDER) {
            fprintf(stderr, "Invalid order: %d .\n\n", order);
            usage_3();
            exit(EXIT_FAILURE);
        }
    }

    license_notice();
    usage_1();  
    usage_2();

    if (argc > 2) {
        input_file = argv[2];
        open_db(input_file);
        /*
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%d\n", &input);
            root = insert_tree(root, input, input);
        }
        fclose(fp);
        print_tree(root);
        */
    }

    printf("Hi\n");
    //printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%" "l" "d", &input);
            printf("Delete start\n");
            delete(input);
            printf("Delete done.\n");
            /*
            scanf("%d", &input);
            root = delete_tree(root, input);
            print_tree(root);
            */
            break;
        case 'i':
            scanf("%" "l" "d", &input);
            scanf("%s", value);
            printf("Insert start\n");
            insert(input, value);
            printf("Insert done.\n");
            /*
            scanf("%d", &input);
            root = insert_tree(root, input, input);
            print_tree(root);
            */
            break;
        case 'f':
        case 'p':
            scanf("%" "l" "d", &input);
            printf("Find start.\n"); 
            if(find(input) != NULL)
                printf("%s\n", find(input));
            printf("Find done.\n");
            /*
            scanf("%d", &input);
            find_and_print(root, input, instruction == 'p');
            */
            break;
        case 'r':
            scanf("%" "l" "d", &input);
            scanf("%d", &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            find_and_print_range(root, input, range2, instruction == 'p');
            break;
        case 'l':
            print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(root);
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        case 'x':
            if (root)
                root = destroy_tree(root);
            print_tree(root);
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        //printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}
