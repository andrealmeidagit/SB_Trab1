#include "semantic_analyser.h"
using namespace std;

/*
* SEMANTIC ANALYSER
* Inputs: tokenlist and labellist
* Output: number of semantic errors
* -----------------------------------------
* Cases analysed:
* - multiple declarations of label;
* - section errors (missing section/wrong order/too many sections)
* - missing labels from data section
* - unused label from data section (warning)
* - Data definition declared as Text label
* - Absent & wrong section TEXT labels
* - constant redefinitions
* - division by zero
* - directive or mnemonic in the wrong section
*/

/*
especificações de roteiro

(X)    – declarações e rótulos ausentes;
(X)    – declarações e rótulos repetidos;
(/)    – pulo para rótulos inválidos; – pulo para seção errada;
(X)    – diretivas inválidas;
(X)    – instruções inválidas;
(X)    – diretivas ou instruções na seção errada;
(X)    – divisão por zero (para constante);
(X)    – instruções com a quantidade de operando inválida; – tokens inválidos;
(X)    – dois rótulos na mesma linha;
(X)    – seção TEXT faltante;
(X)    – seção inválida;
(X)    – tipo de argumento inválido;
(X)    – modificação de um valor constante;

*/


int semantic_analyser(list <Token> & tokenlist, list <Token> & labellist){
    list <Token> datalist, textlist;
    list<Token>::iterator text_it, data_it;
    int err = 0;
    int hasdatasec = 0;
    err+=duplicate_label(labellist);
    err+=section_placement(tokenlist, text_it, data_it, hasdatasec);
    /*
    if (err == 0 && hasdatasec){
        err+=check_symbols_from_data(tokenlist, data_it);
    } else if (err == 0){
        err+=check_for_data_need(tokenlist);
    }
    err+=defaslabel(tokenlist, data_it);
    err+=invalid_label(tokenlist, data_it);
    */
    err+=nolabel(tokenlist, data_it);

    err+=begendexist(tokenlist);
    err+=labelexist(tokenlist, labellist);

    //err+=const_cases(tokenlist, data_it);
    err+=wrong_section(tokenlist, data_it);

    return err;
}

int begendexist(list <Token> & tokenlist){
    list<Token>::iterator it;
    int err = 0;
    int fbegin = 0;
    int fend = 0;

    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if (it->type == TT_DIRECTIVE && it->addit_info == DIR_BEGIN){
            fbegin = 1;
            if (solo){
                cerr << "Semantic Error @ Line " << it->line_number << " - the archive can't have BEGIN directive." << endl;
                pre_error = 1;
                err++;
            }
        }
        if (it->type == TT_DIRECTIVE && it->addit_info == DIR_END){
            fend = 1;
            if (solo){
                cerr << "Semantic Error @ Line " << it->line_number << " - the archive can't have END directive." << endl;
                pre_error = 1;
                err++;
            }
            if (!fbegin){
                cerr << "Semantic Error @ Line " << it->line_number << " - END directive before BEGIN." << endl;
                pre_error = 1;
                err++;
            }
        }
    }
    if (!solo){
        if (!fbegin){
            cerr << "Semantic Error - missing BEGIN directive." << endl;
            pre_error = 1;
            err++;
        }
        if (!fend){
            cerr << "Semantic Error - missing END directive." << endl;
            pre_error = 1;
            err++;
        }
    }

    return err;
}

int labelexist (list <Token> & tokenlist, list <Token> & labellist){
    list<Token>::iterator itt, itl;
    int err = 0;
    int flag = 0;

    for (itt = tokenlist.begin(); itt != tokenlist.end(); itt++){
        if (itt->type == TT_OPERAND){
            for (itl = labellist.begin(); itl != labellist.end(); itl++){
                if (itt->str == itl->str){
                    flag = 1;
                    break;
                }
            }
            if (!flag) {
                cerr << "Semantic Error @ Line " << itt->line_number << " - no definition of label (" << itt->str << ") found." << endl;
                pre_error = 1;
                err++;
            }
            flag = 0;
        }
    }

    return err;
}

int duplicate_label (list <Token> & labellist){
    list<Token>::iterator it, aux, end;
    int err = 0;
    end=labellist.end();
    end--;              //stops loop one before
    for (it=labellist.begin(); it != end; it++){
        aux=it;
        aux++;
        while(aux != labellist.end()){
            if (it->str == aux->str){
                cerr << "Semantic Error @ Line " << aux->line_number << " - multiple declarations of '" << aux->str <<"'." << endl;
                cerr << "\t\tPrevious declaration @ line " << it->line_number << "." << endl;
                pre_error = 1;
                err++;
            }
            aux++;
        }
    }
    return err;
}


int section_placement (list <Token> & tokenlist, list<Token>::iterator & text, list<Token>::iterator & data, int & hasdatasec){
    list<Token>::iterator it = tokenlist.begin();
    int err = 0;
    int count = 0;
    while (it != tokenlist.end()){
        if (it->type == TT_DIRECTIVE && it->addit_info == DIR_SECTION && it->flag != -1){     //if section
            it++;
            if (count == 0){    //if first section
                if (!(it->type == TT_DIRECTIVE && it->addit_info == DIR_TEXT)){     //if not section text
                    fprintf(stderr, "Semantic error @ line %d - Expected 'TEXT' section!\n", it->line_number);
                    pre_error = 1;
                    err++;
                }else{
                    text = it;
                }
            }else if (count == 1 && err == 0){      //second section
                if (!(it->type == TT_DIRECTIVE && (it->addit_info == DIR_DATA || it->addit_info == DIR_BSS))){     //not data section
                    fprintf(stderr, "Semantic error @ line %d - Expected 'DATA' or 'BSS' section!\n", it->line_number);
                    pre_error = 1;
                    err++;
                }else{
                    data = it;
                }
            }else if (count == 2 && err == 0){      //third section
                if (!(it->type == TT_DIRECTIVE && (it->addit_info == DIR_DATA || it->addit_info == DIR_BSS))){     //not data section
                    fprintf(stderr, "Semantic error @ line %d - Expected 'DATA' or 'BSS' section!\n", it->line_number);
                    pre_error = 1;
                    err++;
                }else{
                    data = it;
                }
            }else if (err == 0){    //fourth section
                fprintf(stderr, "Semantic error @ line %d - Too many sections!\n", it->line_number);
                pre_error = 1;
                err++;
            }
            count++;
        }
        it++;
    }
    if (it == tokenlist.end() && count == 0){   //no section
        fprintf(stderr, "Semantic error - No section found!\n");
        pre_error = 1;
        err++;
    }
    if (count == 2){
        hasdatasec = 1;
    }
    return err;
}


int check_symbols_from_data(list <Token> & tokenlist, list<Token>::iterator data_begin){
    int err = 0;
    int i =0;
    list<Token>::iterator it, data_it, aux;
    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if (it->type == TT_OPERAND && it->addit_info != -1 && it->flag != -1){
            for (data_it = data_begin; data_it != tokenlist.end(); data_it++){
                if (data_it->type == TT_LABEL){
                    if (data_it->str.substr(0, data_it->str.find(":")) == it->str){
                        data_it->flag = 100;        //marks data flags that are related to an operand
                        break;
                    }
                }
            }
            if (data_it == tokenlist.end()){
                aux = it;
                aux--;
                if (!(aux->type == TT_MNEMONIC && (aux->addit_info == OP_JMP ||\
                                                aux->addit_info == OP_JMPN ||\
                                                aux->addit_info == OP_JMPP ||\
                                                aux->addit_info == OP_JMPZ))){
                    if (aux->line_number == it->line_number){
                        for (aux = it; aux != tokenlist.begin(); aux--){
                            if (aux->type == TT_DIRECTIVE && aux->addit_info == DIR_MACRO){
                                i=1;
                                break;
                            }
                        }
                        for (aux = it; aux != tokenlist.begin(); aux++){
                            if (aux->type == TT_DIRECTIVE && aux->addit_info == DIR_ENDMACRO){
                                i++;
                                break;
                            }
                        }
                        if(i!=2){
                            //cout << "Token: " << it->str << "..   \tLine: " << it->line_number << "   \tPosition in line: " << it->token_pos_il << "    \tType: " << it->type << "        \taddt_info: " << it->addit_info << "    \tflag: " << it->flag << "     \tinfo str: " << it->info_str << endl;  //print list element
                            fprintf(stderr, "Semantic error @ line %d - Argument '%s' not declared in DATA section.\n", it->line_number, it->str.c_str());
                            pre_error = 1;
                            err++;
                        }
                    }
                }
            }
        }
    }
    for (data_it = data_begin; data_it != tokenlist.end(); data_it++){
        if (data_it->type == TT_LABEL && data_it->flag != 100 && data_it->addit_info != -1){
            fprintf(stderr, "Warning @ line %d - Unused '%s' argument.\n", data_it->line_number, data_it->str.c_str());
            //pre_error = 1;
            //err++;
        }
    }
    return err;
}


int check_for_data_need(list <Token> & tokenlist){
    int err = 0;
    int i = 0;
    list<Token>::iterator it, aux;
    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if (it->type == TT_OPERAND && it->addit_info != -1 && it->flag != -1){
            aux = it;
            aux--;
            if (!(aux->type == TT_MNEMONIC && (aux->addit_info == OP_JMP ||\
                                            aux->addit_info == OP_JMPN ||\
                                            aux->addit_info == OP_JMPP ||\
                                            aux->addit_info == OP_JMPZ))){
                fprintf(stderr, "Semantic error @ line %d - No DATA section - Argument '%s' not declared.\n", it->line_number, it->str.c_str());
                pre_error = 1;
                err++;
                i = 1;
            }
        }
    }
    if (i==1){
        fprintf(stderr, "Semantic error - Expected 'DATA' section!\n");
    }
    return err;
}


int defaslabel(list<Token> & tokenlist, list<Token>::iterator data_it){
    int err = 0;
    list<Token>::iterator it, newit, aux, auxx;
    int i = 0;
    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if (it->type == TT_OPERAND && it->addit_info != -1 && it->flag != -1){
            aux = it;
            aux--;
            if (aux->line_number == it->line_number){
                if (!(aux->type == TT_MNEMONIC && (aux->addit_info == OP_JMP ||\
                                                aux->addit_info == OP_JMPN ||\
                                                aux->addit_info == OP_JMPP ||\
                                                aux->addit_info == OP_JMPZ))){
                    for (newit=tokenlist.begin();newit != data_it; newit++){
                        if ( (newit->str.substr(0, newit->str.find(":")) == it->str) && \
                                            (newit->type == TT_LABEL) && \
                                            (newit->line_number != it->line_number) && \
                                            (newit->token_pos_il != it->token_pos_il) ){
                            for (auxx = it; auxx != tokenlist.begin(); auxx--){
                                if (auxx->type == TT_DIRECTIVE && auxx->addit_info == DIR_MACRO){
                                    i=1;
                                    break;
                                }
                            }
                            for (auxx = it; auxx != tokenlist.begin(); auxx++){
                                if (auxx->type == TT_DIRECTIVE && auxx->addit_info == DIR_ENDMACRO){
                                    i++;
                                    break;
                                }
                            }
                            if (i!=2){
                                fprintf(stderr, "Semantic error @ line %d - Definition from line %d (%s) declared as TEXT label in line %d.\n", it->line_number, it->line_number, it->str.c_str(), newit->line_number);
                                pre_error = 1;
                                err++;
                            }
                        }
                    }
                }
            }
        }
    }
    return err;
}

int nolabel(list<Token> & tokenlist, list<Token>::iterator data_it){
    int err = 0;
    list<Token>::iterator it, newit, aux;
    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if (it->type == TT_OPERAND && it->addit_info != -1 && it->flag != -1){
            aux = it;
            aux--;
            if (aux->type == TT_MNEMONIC && (aux->addit_info == OP_JMP ||\
                                            aux->addit_info == OP_JMPN ||\
                                            aux->addit_info == OP_JMPP ||\
                                            aux->addit_info == OP_JMPZ)){
                for (newit=tokenlist.begin();newit != data_it; newit++){
                    if ( (newit->str.substr(0, newit->str.find(":")) == it->str) && \
                                        (newit->type == TT_LABEL) && \
                                        (newit->line_number != it->line_number) && \
                                        (newit->token_pos_il != it->token_pos_il) ){
                        break;
                    }
                }
                if (newit == data_it){
                    fprintf(stderr, "Semantic error @ line %d - Label '%s' not declared in TEXT section.\n", it->line_number, it->str.c_str());
                    pre_error = 1;
                    err++;
                    while (newit != tokenlist.end()){
                        newit++;
                        if ( (newit->str.substr(0, newit->str.find(":")) == it->str) && \
                                            (newit->type == TT_LABEL) && \
                                            (newit->line_number != it->line_number) && \
                                            (newit->token_pos_il != it->token_pos_il) ){
                            fprintf(stderr, "Semantic error @ line %d - Label '%s' defined in DATA section, but a TEXT section label was expected (previous declaration @ line %d).\n", newit->line_number, it->str.c_str(), it->line_number);
                            pre_error = 1;
                            err++;
                        }
                    }
                }
            }
        }
    }
    return err;
}

int invalid_label(list<Token> & tokenlist, list<Token>::iterator data_it){
    int err = 0;
    list<Token>::iterator it, newit, aux;
    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if (it->type == TT_OPERAND && it->addit_info != -1 && it->flag != -1){
            aux = it;
            aux--;
            if (aux->type == TT_MNEMONIC && (aux->addit_info == OP_JMP ||\
                                            aux->addit_info == OP_JMPN ||\
                                            aux->addit_info == OP_JMPP ||\
                                            aux->addit_info == OP_JMPZ)){
                for (newit=tokenlist.begin();newit != data_it; newit++){
                    if ( (newit->str.substr(0, newit->str.find(":")) == it->str) && \
                                        (newit->type == TT_LABEL) && \
                                        (newit->line_number != it->line_number) && \
                                        (newit->token_pos_il != it->token_pos_il) ){
                        break;
                    }
                }
                if (newit != data_it){
                    aux = newit;
                    aux++;
                    if (aux->type == TT_DIRECTIVE){
                        fprintf(stderr, "Semantic error @ line %d - Jump to invalid Label ('%s' - previous declaration @ line %d).\n", newit->line_number, it->str.c_str(), it->line_number);
                        pre_error = 1;
                        err++;
                    }
                }
            }
        }
    }
    return err;
}


int const_cases(list<Token> & tokenlist, list<Token>::iterator data_it){
    int err = 0;
    list<Token>::iterator it, otherit, aux;
    for (it = tokenlist.begin(); it != tokenlist.end(); it++){
        if ( it->type == TT_MNEMONIC ){
            if (it->addit_info == OP_STORE || it->addit_info == OP_COPY){
                it++;
                for (otherit = data_it; otherit != tokenlist.end(); otherit++){
                    if (otherit->str.substr(0, otherit->str.find(":")) == it->str){
                        aux = otherit;
                        advance (aux, 2);
                        if (aux->line_number == otherit->line_number && aux->type == TT_CONST){
                            fprintf(stderr, "Semantic error @ line %d - Atempt to change constant value ('%s').\n", it->line_number, it->str.c_str());
                            pre_error = 1;
                            err++;
                        }
                    }
                }
            }
            if (it->addit_info == OP_DIV){
                it++;
                for (otherit = data_it; otherit != tokenlist.end(); otherit++){
                    if (otherit->str.substr(0, otherit->str.find(":")) == it->str){
                        aux = otherit;
                        advance (aux, 2);
                        if (aux->line_number == otherit->line_number && aux->type == TT_CONST){
                            if (aux->addit_info == 0){
                                fprintf(stderr, "Semantic error @ line %d - Atempt to divide by zero.\n", it->line_number);
                                pre_error = 1;
                                err++;
                            }
                        }
                    }
                }
            }
        }
    }
    return err;
}


int wrong_section(list<Token> & tokenlist, list<Token>::iterator data_it){
    int err = 0;
    list<Token>::iterator it;
    for (it = tokenlist.begin(); it != data_it; it++){
        if ((it->type == TT_DIRECTIVE) && (it->addit_info == DIR_CONST)){
            fprintf(stderr, "Semantic error @ line %d - Atempt to use the directive '%s' in the wrong section.\n", it->line_number, it->str.c_str());
            pre_error = 1;
            err++;
        }
    }
    while (it != tokenlist.end()){
        if (it->type == TT_MNEMONIC){
            fprintf(stderr, "Semantic error @ line %d - Atempt to use the mnemonic '%s' in the wrong section.\n", it->line_number, it->str.c_str());
            pre_error = 1;
            err++;
        }
        it++;
    }
    return err;
}
