#ifdef _LINUX_
#define advance_node          advance_node__
#define advance_to_pos        advance_to_pos__
#define augment_count         augment_count__
#define char_from_table       char_from_table__  /* OB 2.4.2002 */
#define comment_to_table      comment_to_table__
#define comm_para             comm_para__
#define double_from_table     double_from_table__
#define double_to_table       double_to_table__
#define element_name          element_name__
#define frndm                 frndm_
#define madx                  madx_
#define madx_init             madx_init__
#define f_ctof                f_ctof__
#define get_disp0             get_disp0__
#define get_option            get_option__
#define get_orbit0            get_orbit0__
#define get_string            get_string__
#define get_title             get_title__
#define get_value             get_value__
#define get_variable          get_variable__
#define get_version           get_version__
#define grndm                 grndm_
#define intrac                intrac_
#define mtcond                mtcond_
#define next_constraint       next_constraint__
#define next_global           next_global__
#define next_start            next_start__
#define next_vary             next_vary__
#define node_al_errors        node_al_errors__
#define node_fd_errors        node_fd_errors__
#define node_value            node_value__
#define node_vector           node_vector__
#define plot_option           plot_option__
#define reset_count           reset_count__
#define restart_sequ          restart_sequ__
#define sequence_name         sequence_name__
#define set_option            set_option__
#define set_value             set_value__
#define set_variable          set_variable__
#define string_to_table       string_to_table__
#define table_length          table_length__
#define table_org             table_org__
#define table_range           table_range__
#define track_pteigen         track_pteigen__
#define vector_to_table       vector_to_table__
#define vdot                  vdot_
#define vmod                  vmod_
#endif
#ifndef _LINUX_
#define advance_node          advance_node_
#define advance_to_pos        advance_to_pos_
#define augment_count         augment_count_
#define char_from_table       char_from_table_  /* OB 2.4.2002 */
#define comment_to_table      comment_to_table_
#define comm_para             comm_para_
#define double_from_table     double_from_table_
#define double_to_table       double_to_table_
#define element_name          element_name_
#define frndm                 frndm_
#define madx                  madx_
#define madx_init             madx_init_
#define f_ctof                f_ctof_
#define get_disp0             get_disp0_
#define get_option            get_option_
#define get_orbit0            get_orbit0_
#define get_string            get_string_
#define get_title             get_title_
#define get_variable          get_variable_
#define get_value             get_value_
#define get_version           get_version_
#define grndm                 grndm_
#define intrac                intrac_
#define mtcond                mtcond_
#define next_constraint       next_constraint_
#define next_global           next_global_
#define next_start            next_start_
#define next_vary             next_vary_
#define node_al_errors        node_al_errors_
#define node_fd_errors        node_fd_errors_
#define node_value            node_value_
#define node_vector           node_vector_
#define plot_option           plot_option_
#define reset_count           reset_count_
#define restart_sequ          restart_sequ_
#define sequence_name         sequence_name_
#define set_option            set_option_
#define set_value             set_value_
#define set_variable          set_variable_
#define string_to_table       string_to_table_
#define table_length          table_length_
#define table_org             table_org_
#define table_range           table_range_
#define track_pteigen         track_pteigen_
#define vector_to_table       vector_to_table_
#define vdot                  vdot_
#define vmod                  vmod_
#endif

/* short utility routines */
int is_operand(char c) { return (isalnum(c) || c == '_' || c == '.');}
int is_operator(char c) {return (strchr("-+*/^", c) ? 1 : 0);}
int is_expr_start(char c) {return (strchr("-+(",c) || is_operand(c));}
int max(int a, int b) {return (a > b ? a : b);}
int min(int a, int b) {return (a < b ? a : b);}
int str_pos(const char s[], char c) 
{int i; for (i = 0; i < strlen(s); i++) if (s[i] == c) return i; return -1;}

/* Fortran routines called from C */
extern void dynap_(double*, double*, int*, int*, double*, double*, double*,
                   double*, double*);
extern void mtgeti_(int*, double*, double*);
extern void collect_(int*, double*, double*); /* OB 13.2.2002 */
extern void fortinit_();
extern void gxterm_();
extern void haveit_(double *,double *,double *,double *,int *,int *,
int *,double *,double *,double *,double *,double *,double *);
extern void ibs_();
extern void micit_(double *,char *,double *,double *,double *,int *,float *,
int *,int *,int *,int *,float *,float *,float *,float *,float *,
float *,float *,float *,float *,float *);
extern void mtlmdf_(int*, int*, double*, int*, int*, double*, double*,
                    double*, double*,double*, double*, double*, double*,
                    double*, double*, double*, double*, double*); 
extern void mtmigr_(int*, int*, int*, double*, int*, int*, double*, double*,
                    double*, double*, double*, double*, double*, double*,
                    double*, double*); 
extern void mtsimp_(int*, int*, double*, int*, int*, double*, double*,
                    double*, double*, double*, double*); 
extern void pefill_(int*);
extern void pemima_();
extern void pesopt_(int*);
extern void plotit_(int*);
extern void setup_(double *respx,double *dmat,int *im, 
int *ic, int *nm, int*nc);
extern void survey_();
extern void tmrefe_(double*);
extern void tmrefo_(int*,double*,double*,double*);
extern void trrun_(int*,int*,double*,double*,int*,int*,
                   double*,double*,double*,double*,
                   double*,int*, int*, double*);
extern void twiss_(double*, double*, int*);

/* C routines called from Fortran and C */
int advance_node();
int advance_to_pos(char*, int*);
char* alias(char*);
void augment_count(char*);
int char_from_table(char*, char*, int*, char*); /* OB 2.4.2002 */
void comment_to_table(char*, char*, int*);
void comm_para(char*, int*, int*, int*, int*, double*, char*, int*);
int double_from_table(char*, char*, int*, double*);
void double_to_table(char*, char*, double*);
void element_name(char*, int*);
double frndm();
void get_disp0(double*);
int get_option(char*);
void get_orbit0(double*);
int get_string(char*, char*, char*);
void get_title(char*, int*);
double get_value(char*, char*);
double get_variable(char*);
void get_version(char*, int*);
double grndm();
int intrac();
int next_constraint(char*, int*, int*, double*, double*, double*, double*);
int next_global(char*, int*, int*, double*, double*, double*, double*);
int next_start(double*,double*,double*,double*,double*,double*,double*,
               double*,double*,double*,double*,double*);
int next_vary(char*, int*, double*, double*, double*);
int node_al_errors(double*);
int node_fd_errors(double*);
double node_value(char*);
void node_vector(char*, int*, double*);
double plot_option(char*);
void reset_count(char*);
int restart_sequ();
void sequence_name(char*, int*);
void set_value(char*, char*, double*);
void set_variable(char*, double*);
void string_to_table(char*, char*, char*);
int table_length(char*);
int table_org(char*);
void table_range(char*, char*, int*);
void vector_to_table(char*, char*, int*, double*);

/* C routines called from C */
double act_value(int, struct name_list*);
int act_special(int, char*);
int add_drifts(struct node*, struct node*);
void add_to_command_list(char*, struct command*, struct command_list*, int);
void add_to_command_list_list(char*, struct command_list*, 
                              struct command_list_list*);
void add_to_constraint_list(struct constraint*, struct constraint_list*);
void add_to_el_list(struct element*, int, struct el_list*, int);
void add_to_macro_list(struct macro*, struct macro_list*);
int add_to_name_list(char*, int, struct name_list*);
void add_to_node_list(struct node*, int, struct node_list*);
void add_to_sequ_list(struct sequence*, struct sequence_list*);
void add_to_table_list(struct table*, struct table_list*);
void add_to_var_list(struct variable*, struct var_list*, int);
void adjust_beam();
void all_node_pos(struct sequence*);
int attach_beam(struct sequence*);
int belongs_to_class(struct element*, char*);
char* buffer(char*);
struct in_cmd* buffered_cmd(struct in_cmd*);
void buffer_in_cmd(struct in_cmd*);
int char_cnt(char, char*);
int char_p_pos(char*, struct char_p_array*);
void check_table(char*);
struct char_p_array* clone_char_p_array(struct char_p_array*);
struct command* clone_command(struct command*);
struct command_parameter* clone_command_parameter(struct command_parameter*);
struct double_array* clone_double_array(struct double_array*);
struct element* clone_element(struct element*);
struct expression* clone_expression(struct expression*);
struct expr_list* clone_expr_list(struct expr_list*);
struct int_array* clone_int_array(struct int_array*);
struct name_list* clone_name_list(struct name_list*);
struct node* clone_node(struct node*);
void copy_name_list(struct name_list*, struct name_list*);
int cmd_match(int, char**, int*, int*);
void complete_twiss_table(struct table*);
char* compound(char*, int);
struct expression* compound_expr(struct expression*, double, char*,
                                 struct expression*, double);
void control(struct in_cmd*);
void conv_char(char*, struct int_array*);
void conv_sixtrack(struct in_cmd*);
void correct_correct(struct in_cmd*);
void correct_getorbit(struct in_cmd*);
void correct_putorbit(struct in_cmd*);
void correct_usekick(struct in_cmd*);
void correct_usemonitor(struct in_cmd*);
void deco_init();
int decode_command();
int decode_par(struct in_cmd*, int, int, int, int);
struct char_array* delete_char_array(struct char_array*);
struct char_p_array* delete_char_p_array(struct char_p_array*, int);
struct command* delete_command(struct command*);
struct command_list* delete_command_list(struct command_list*);
struct command_parameter* delete_command_parameter(struct command_parameter*);
struct command_parameter_list* 
       delete_command_parameter_list(struct command_parameter_list*);
struct double_array* command_par_array(char*, struct command*);
struct expression* command_par_expr(char*, struct command*);
char* command_par_string(char*, struct command*);
double command_par_value(char*, struct command*);
struct constraint* delete_constraint(struct constraint*);
struct constraint_list* delete_constraint_list(struct constraint_list*);
struct element* delete_element(struct element*);
struct el_list* delete_el_list(struct el_list*);
struct expression* delete_expression(struct expression*);
struct expr_list* delete_expr_list(struct expr_list*);
struct double_array* delete_double_array(struct double_array*);
struct in_cmd* delete_in_cmd(struct in_cmd*);
struct int_array* delete_int_array(struct int_array*);
struct macro* delete_macro(struct macro*);
struct name_list* delete_name_list(struct name_list*);
struct node* delete_node(struct node*);
struct node* delete_node_ring(struct node*);
struct node_list* delete_node_list(struct node_list*);
struct sequence_list* delete_sequence_list(struct sequence_list*);
struct variable* delete_variable(struct variable*);
struct var_list* delete_var_list(struct var_list*);
struct table* delete_table(struct table*);
double double_from_expr(char**, int, int);
void down_unit(char*);
void dump_constraint_list(struct constraint_list*);
void dump_char_array(struct char_array*);
void dump_char_p_array(struct char_p_array*);
void dump_command(struct command*);
void dump_command_parameter(struct command_parameter*);
void dump_constraint(struct constraint*);
void dump_element(struct element*);
void dump_element_array(struct element**);
void dump_el_list(struct el_list*);
void dump_expression(struct expression*);
void dump_exp_sequ(struct sequence*, int);
void dump_in_cmd(struct in_cmd*);
void dump_int_array(struct int_array*);
void dump_macro(struct macro*);
void dump_macro_list(struct macro_list*);
void dump_name_list(struct name_list*);
void dump_node(struct node*);
void dump_sequ(struct sequence*, int);
void dump_variable(struct variable*);
void dynap_tables_create(struct in_cmd*);
double element_value(struct node*, char*);
double el_par_value(char*, struct element*);
int element_vector(struct element*, char*, double*);
void enter_element(struct in_cmd*);
void enter_elm_reference(struct in_cmd*, struct element*, int);
void enter_sequ_reference(struct in_cmd*, struct sequence*);
void enter_sequence(struct in_cmd*);
void enter_variable(struct in_cmd*);
void exec_assign(struct in_cmd*);
void exec_beam(struct in_cmd*, int);
void exec_call(struct in_cmd*);
void exec_command();
void exec_dump(struct in_cmd*);
void exec_dumpsequ(struct in_cmd*);
void exec_help(struct in_cmd*);
void exec_macro(struct in_cmd*, int);
void exec_option();
void exec_plot(struct in_cmd*);
void exec_print(struct in_cmd*);
void exec_save(struct in_cmd*);
void exec_savebeta();
void exec_show(struct in_cmd*);
void expand_line(struct char_p_array*);
struct node* expand_node(struct node*, struct sequence*, struct sequence*, 
                         double);
void expand_sequence(struct sequence*);
void export_element(struct element*, struct el_list*, FILE*);
void export_elem_8(struct element*, struct el_list*, FILE*);
void export_el_def(struct element*, char*);
void export_el_def_8(struct element*, char*);
void export_el_par(struct command_parameter*, char*);
void export_el_par_8(struct command_parameter*, char*);
void export_sequence(struct sequence*, FILE*);
void export_sequ_8(struct sequence*, FILE*);
void export_variable(struct variable*, FILE*);
void export_var_8(struct variable*, FILE*);
double expression_value(struct expression*, int);
void fatal_error(char*, char*);
void fill_beta0(struct command*, struct node*);
void fill_constraint_list(int, struct command*, struct constraint_list*);
void fill_elem_var_list(struct element*, struct el_list*, struct var_list*);
void fill_expr_list(char**, int, int, struct expr_list*);
void fill_expr_var_list(struct el_list*, 
                        struct expression*, struct var_list*);
void fill_orbit_table(struct table*, struct table*);
void fill_par_var_list(struct el_list*, 
                       struct command_parameter*, struct var_list*);
void fill_sequ_list(struct sequence*, struct sequence_list*);
void fill_sequ_var_list(struct sequence_list*, struct el_list*, 
                        struct var_list*);
struct command* find_command(char*, struct command_list*);
struct command_list* find_command_list(char*, struct command_list_list*);
struct element* find_element(char*, struct el_list*);
struct variable* find_variable(char*, struct var_list*);
int force_pos(char*);
void ftoi_array(struct double_array*, struct int_array*);
void madx();
void madx_finish();
void madx_init();
void madx_start();
void get_bracket_range(char*, char, char, int*, int*);
void get_bracket_t_range(char**, char, char, int, int, int*, int*);
void get_defined_commands();
void get_defined_constants();
struct element* get_drift(double);
void get_inverted_forces();
char* get_new_name();
double get_node_pos(struct node*, struct sequence*);
int get_node_count(struct node*);
int get_ex_range(char*, struct sequence*, struct node**);
int get_range(char*, struct sequence*, struct node**);
double get_refpos(struct sequence*);
int get_select_ex_ranges(struct sequence*,struct command_list*);
int get_select_ranges(struct sequence*,struct command_list*);
void get_select_t_ranges(struct command_list*, struct table*);
int square_to_colon(char*);
int get_stmt(FILE*);
int get_table_range(char*, struct table* t, int*);
void grow_char_p_array(struct char_p_array*);
void grow_command_list(struct command_list*);
void grow_command_list_list(struct command_list_list*);
void grow_command_parameter_list(struct command_parameter_list*);
void grow_constraint_list(struct constraint_list*);
void grow_double_array(struct double_array*);
void grow_el_list(struct el_list*);
void grow_expr_list(struct expr_list*);
void grow_in_buff_list(struct in_buff_list*);
void grow_in_cmd_list(struct in_cmd_list*);
void grow_int_array(struct int_array*);
void grow_macro_list(struct macro_list*);
void grow_name_list(struct name_list*);
void grow_node_list(struct node_list*);
void grow_sequence_list(struct sequence_list*);
void grow_table(struct table*);
void grow_table_list(struct table_list*);
void grow_var_list(struct var_list*);
double hidden_node_pos(char*, struct sequence*);
void init55(int);
void irngen();
int in_spec_list(char*);
int int_in_array(int, int, int*);
void insert_elem(struct sequence*, struct node*);
void install_one(struct element*, char*, double, struct expression*, double);
char* join(char**, int);
char* join_b(char**, int);
double line_nodes(struct char_p_array*);
void link_in_front(struct node*, struct node*);
int loc_expr(char**, int, int, int*);
int logic_expr(int, char**);
int log_val(char*, struct command*);
void main_input();
struct constraint* make_constraint(int, struct command_parameter*);
struct element* make_element(char*, char*, struct command*, int);
void make_elem_node(struct element*, int);
struct expression* make_expression(int, char**);
int make_line(char*);
int make_macro(char*);
void make_occ_list(struct sequence*);
struct table* make_optics_table(struct table*);
void make_sequ_from_line(char*);
void make_sequ_node(struct sequence*, int);
char* make_string_variable(char*);
struct table* make_table(char*, char tc[][TABLE_KEY], int*, int);
void makethin(struct in_cmd*);
void match_action(struct in_cmd*);
void match_cell(struct in_cmd*);
void match_constraint(struct in_cmd*);
void match_couple(struct in_cmd*);
void match_end(struct in_cmd*);
void match_fix(struct in_cmd*);
void match_gweight(struct in_cmd*);
void match_global(struct in_cmd*);
int match_input(struct command*);  /* OB 23.1.2002 */
void match_level(struct in_cmd*);
void match_match(struct in_cmd*);
void match_rmatrix(struct in_cmd*);
void match_tmatrix(struct in_cmd*);
void match_vary(struct in_cmd*);
void match_weight(struct in_cmd*);
void mtcond(int*, int*, double*, int*);
double mult_par(char*, struct element*);
void mycpy(char*, char*);
void* mycalloc(char*, size_t, size_t);
void* mymalloc(char*, size_t);
char* mystrchr(char*, char);
char* mystrstr(char*, char*);
void my_repl(char*, char*, char*, char*);
int name_list_pos(char*, struct name_list*);
int name_tab(char*, struct name_list*);
struct in_buff_list* new_in_buff_list(int);
struct char_array* new_char_array(int);
struct char_array_list* new_char_array_list(int);
struct char_p_array* new_char_p_array(int);
struct command* new_command(char*, int, int, char*, char*, int, int);
struct command_list* new_command_list(int);
struct command_list_list* new_command_list_list(int);
struct constraint* new_constraint(int);
struct constraint_list* new_constraint_list(int);
struct in_buffer* new_in_buffer(int);
struct in_cmd* new_in_cmd(int);
struct in_cmd_list* new_in_cmd_list(int);
struct command_parameter* new_command_parameter(char*, int);
struct command_parameter_list* new_command_parameter_list(int);
struct double_array* new_double_array(int);
struct element* new_element(char*);
struct el_list* new_el_list(int);
struct expr_list* new_expr_list(int);
struct expression* new_expression(char*, struct int_array*);
struct int_array* new_int_array(int);
struct macro* new_macro(int, int, int);
struct macro_list* new_macro_list(int);
struct name_list* new_name_list(int);
struct node* new_elem_node(struct element*, int);
struct node* new_node(char*);
struct node_list* new_node_list(int);
struct sequence* new_sequence(char*, int);
struct sequence_list* new_sequence_list(int);
struct node* new_sequ_node(struct sequence*, int);
struct table* new_table(char*, int, struct name_list*);
struct table_list* new_table_list(int);
struct variable* new_variable(char*, double, int, int, struct expression*,
                              char*);
struct var_list* new_var_list(int);
int next_char(char, char**, int, int);
char next_non_blank(char*);
int next_non_blank_pos(char*);
char* noquote(char*);
void out_table(char*, struct table*, char*);
int par_present(char*, struct command*, struct command_list*);
int par_out_flag(char*, char*);
int pass_select(char*, struct command*);
char* permbuff(char*);
int polish_expr(int, char**);
double polish_value(struct int_array*);
void prepare_table_file(struct table*, struct command_list*);
void pre_split(char*, char*, int);
void print_command(struct command*);
void print_command_parameter(struct command_parameter*);
void print_table(struct table*);
void print_value(struct in_cmd*);
void pro_match(struct in_cmd*);
void pro_node(int, double);
void process();
void pro_correct(struct in_cmd*);
void pro_error(struct in_cmd*);
void pro_ibs(struct in_cmd*);
void pro_input(char*);
void pro_survey(struct in_cmd*);
void pro_track(struct in_cmd*);
void pro_twiss();
void put_info(char*, char*);
struct table* read_table(struct in_cmd*);
int remove_colon(char**, int, int);
void remove_from_command_list(char*, struct command_list*);
int remove_from_name_list(char*, struct name_list*);
void remove_from_node_list(struct node*, struct node_list*);
int remove_one(struct node*);
void remove_range(char*, char*, char*);
void remove_upto(char*, char*);
void replace(char*, char, char);
void replace_lines(struct macro*, int, char**);
void replace_one(struct node*, struct element*);
void resequence_nodes(struct sequence*);
void reset_errors(struct sequence*);
double rfc_slope();
int scan_expr(int, char**);
void scan_in_cmd(struct in_cmd*);
void seq_cycle(struct in_cmd*);
void seq_edit(struct in_cmd*);
void seq_edit_main(struct in_cmd*);
void seq_end(struct in_cmd*);
void seq_flatten(struct sequence*);
void seq_install(struct in_cmd*);
void seq_move(struct in_cmd*);
void seq_reflect(struct in_cmd*);
void seq_replace(struct in_cmd*);
void seq_remove(struct in_cmd*);
void set_command_par_value(char*, struct command*, double);
void set_defaults(char*);
int set_enable(char*, struct in_cmd*);
void set_new_position(struct sequence*);
void set_node_bv(struct sequence*);
void set_option(char*, int*);
void set_range(char*, struct sequence*);
void set_selected_columns(struct table*, struct command_list*);
void set_selected_elements();
void set_selected_errors();
void set_selected_rows(struct table*, struct command_list*);
void set_twiss_deltas(struct command*);
void set_sub_variable(char*, char*, struct in_cmd*);
void show_beam(char*);
double simple_double(char**, int, int);
int simple_logic_expr(int, char**);
char* spec_join(char**, int); /* puts table() argument back for output */
int mysplit(char*, struct char_p_array*);
char* stolower(char*);  /* string to lower case in place */
void stolower_nq(char*);  /* string to lower case in place except quotes */
char* stoupper(char*);  /* string to upper case in place */
void store_beta0(struct in_cmd*);
void store_command_def(char*);
struct command_parameter* store_comm_par_def(char**, int, int);
void store_comm_par_value(char*, double, struct command*);
void store_savebeta(struct in_cmd*);
void store_select(struct in_cmd*);
int string_cnt(char, int, char**);
char* strip(char*);
void supp_char(char, char*);
int supp_lt(char*);
char* supp_tb(char*);
double table_value();
int table_row(struct table*, char*);
int tab_name_code(char*, char*);
void time_stamp(char*);
double tgrndm(double);
char* tmpbuff(char*);
void track_dynap(struct in_cmd*);
void track_end(struct in_cmd*);
void track_observe(struct in_cmd*);
void track_pteigen(double*);
void track_run(struct in_cmd*);
void track_ripple(struct in_cmd*);
void track_start(struct command*);
void track_tables_create(struct in_cmd*);
void track_tables_dump();
void track_track(struct in_cmd*);
int twiss_input(struct command*);
void update_beam();
void update_element(struct element*, struct command*);
void update_node_constraints(struct node*, struct constraint_list*);
void update_sequ_constraints(struct sequence*, struct constraint_list*);
void update_vector(struct expr_list*, struct double_array*);
void use_sequ(struct in_cmd*);
double variable_value(struct variable*);
double vdot(int*, double*, double*);
double vmod(int*, double*);
void warning(char*, char*);
void write_elems(struct el_list*, FILE*);
void write_elems_8(struct el_list*, FILE*);
void write_nice(char*, FILE*);
void write_nice_8(char*, FILE*);
void write_sequs(struct sequence_list*, FILE*);
void write_table(struct table*, char*);
void write_vars(struct var_list*, FILE*);
void write_vars_8(struct var_list*, FILE*);
int zero_string(char*);

/* define orbit correction routines */
void pro_correct(struct in_cmd* cmd);
int  pro_correct_getcommands(struct in_cmd* cmd);
int  pro_correct_gettables(int ip);
int  pro_correct_getorbit(struct in_cmd* cmd);
void pro_correct_prtwiss();
void pro_correct_write_cocu_table();
void pro_correct_fill_corr_table(int ip , char *name, double old, double new);
void pro_correct_make_corr_table();
void pro_correct_write_results(double *monvec, double *resvec, double *corvec, int *nx, int *nc, int *nm, int imon, int icor, int ip);
void pro_correct_make_mon_table();
void pro_correct_fill_mon_table(int ip ,char *name, double old, double new);
int pro_correct_getactive(int ip, int *nm, int *nx, int *nc, double *corvec, double *monvec,char *conm);
double crms(double *r, int m);


double* pro_correct_response_ring(int ip, int nc, int nm);
double* pro_correct_response_line(int ip, int nc, int nm);

/* C wrapper to allocate memory for Fortran77 */
void c_micit(double *,char *,double *,double *,double *,int *,float,int,int,int);
void c_haveit(double *,double *,double *,double *,int *,int,int);

/* define error routines */
double cprp(double *r, int m);
double copk(double *r, int m);
double fact(int );
void pro_error(struct in_cmd* cmd);
void error_ealign(struct in_cmd* cmd);
void error_efield(struct in_cmd* cmd);
void error_efcomp(struct in_cmd* cmd);
void error_eoption(struct in_cmd* cmd);
void error_eprint(struct in_cmd* cmd);
void error_esave(struct in_cmd* cmd);
void f_ctof(int *j, char *string, int *nel);
void pro_error_make_efield_table();

/* Global structure variables by type (alphabetic) */

struct char_array_list* char_buff; /* buffer for all sorts of strings */
struct char_p_array* tmp_p_array;  /* temporary buffer for splits */
struct char_p_array* tmp_l_array;  /* temporary buffer for special commands */
struct char_p_array* line_buffer;  /* buffer for line expansion */

struct command* current_beam = NULL;    /* current reference beam */
struct command* probe_beam = NULL;      /* current beam */
struct command* options = NULL;         /* current options */
struct command* plot_options = NULL;    /* current plot options */
struct command* current_error = NULL;   /* current error command */
struct command* current_correct = NULL; /* current correct command */
struct command* current_ibs = NULL;     /* current ibs command */
struct command* current_survey = NULL;  /* current survey command */
struct command* current_twiss = NULL;   /* current twiss command */
struct command* current_command = NULL; /* current command clone */
struct command* current_gweight = NULL; /* current gweight clone */
struct command* current_weight = NULL;  /* current weight clone */
struct command* current_match = NULL;   /* OB 23.1.2002: current match comm. */

struct command_list* beam_list;         /* list of all beam commands */
struct command_list* beta0_list;        /* list of user defined beta0s */
struct command_list* defined_commands;  /* from dictionary */
struct command_list* error_select;      /* current error select commands */
struct command_list* optics_select;     /* current optics select commands */
struct command_list* optics_list;       /* list of optics command/sequence */
struct command_list* savebeta_list;
struct command_list* seqedit_select;    /* current seqedit select commands */
struct command_list* slice_select;      /* current slice select commands */
struct command_list* stored_commands;   /* list of stored commands */
struct command_list* stored_match_var;  /* list of match vary commands */
struct command_list* stored_track_start;/* list of track start commands */

struct command_list_list* table_select; /* list of all table select lists */

struct constraint_list* comm_constraints; /* for each constraint command */
struct double_array* cat_doubles;    /* Polish: constant values */
struct double_array* doubles;        /* doubles buffer */
struct double_array* twiss_deltas;   /* for deltap loop in twiss command */
struct double_array* vary_vect;      /* for matching */
struct double_array* vary_dvect;     /* for matching */
struct double_array* fun_vect;       /* for matching */
struct double_array* match_work[MATCH_WORK];  /* work space for matching */

struct el_list* drift_list;
struct el_list* element_list;
struct el_list* base_type_list;
struct el_list* selected_elements;

struct in_buff_list* in;           /* list of all active input buffers */
struct in_buff_list* pro;          /* list of active processing buffers */

struct int_array* deco;       /* Polish: coded expression */
struct int_array* cat;        /* Polish: catgories */
struct int_array* d_var;      /* Polish: variable references */
struct int_array* oper;       /* Polish: operator references */
struct int_array* func;       /* Polish: function references */
struct int_array* s_range;    /* starts of ranges */
struct int_array* e_range;    /* ends of ranges */

struct in_cmd* this_cmd;      /* contains command just read */
struct in_cmd* local_twiss[2] = {NULL, NULL}; /* OB 1.2.2002 */

struct in_cmd_list* buffered_cmds;

struct macro_list* line_list;
struct macro_list* macro_list;

struct name_list* expr_chunks;
struct name_list* inverted_forces;
struct name_list* occ_list;

struct node* prev_node;
struct node* current_node = NULL;
struct node* debug_node = NULL;

struct node_list* selected_ranges; /* filled by some select commands */

struct sequence* current_sequ;  /* pointer to currently used sequence */
struct sequence* edit_sequ;     /* pointer to sequence being edited */

struct sequence_list* sequences;    /* pointer to sequence list */
struct sequence_list* match_sequs;  /* pointer to sequence list for match */

struct table* ibs_table;          /* current ibs table */
struct table* summ_table;         /* current twiss summary table */
struct table* twiss_table;        /* current twiss table */
struct table* survey_table;       /* current survey table */
struct table* corr_table;         /* corrector table after orbit correction */
struct table* mon_table;          /* monitor table after orbit correction */
struct table* orbit_table;        /* orbit positions at monitors */

struct table_list* optics_tables; /* contains optics tables from last twiss */
struct table_list* table_register; /* contains all tables */

struct variable* current_variable = NULL; /* set by act_value (table access) */
struct var_list* variable_list;

struct orb_cor* correct_orbit; /* information and links for orbit correction */

struct table* efield_table;       /* field errors in table form  */
FILE* fddata;
FILE* fcdata;
FILE* ftdata;

FILE* debug_file;              /* for debug output */
FILE* stamp_file;              /* for debug output */
FILE* out_file;                /* for table output */
FILE* prt_file;                /* for echo output */
FILE* tab_file;                /* for table input */

/* Global simple variables by type */

char quote;                       /* current open single or double quote */
char  tmp_key[NAME_L],
      c_dummy[AUX_LG],
      c_join[AUX_LG],
      l_dummy[AUX_LG],
      work[AUX_LG],
      l_work[AUX_LG];

char blank[] = "    ";
char none[] = "none";
char myversion[] = "MAD-X 1.00";
char one_string[] = "1";

char* aux_char_pt;               /* for debug purposes */
char* exx;
char* current_link_group;
char* title = NULL;
char* match_seqs[2];             /* OB 23.1.2002   */
char* match_beta[2];             /* OB 23.1.2002   */

double pi, twopi, degrad, raddeg, e, clight, hbar;
double penalty;
double match_tol;
double orbit0[6];
double disp0[6];
double track_deltap=0;
double oneturnmat[36];

const double zero = 0;
const double one = 1;
const double two = 2;
const double ten_p_3 = 1.e3;
const double ten_p_6 = 1.e6;
const double ten_p_9 = 1.e9;
const double ten_p_12 = 1.e12;
const double ten_m_3 = 1.e-3;
const double ten_m_6 = 1.e-6;
const double ten_m_9 = 1.e-9;
const double ten_m_12 = 1.e-12;
const double ten_m_15 = 1.e-15;
const double ten_m_16 = 1.e-16;
const double ten_m_19 = 1.e-19;

int add_error_opt = 0;      /* ADD error option, set with eoption */
int assign_start = 0;       /* flag for multiple assign statements */
int aux_count = 0;          /* for debug purposes */
int beam_info = -1;         /* flag to print beam information once */
int c_range_end;            /* node count of current range end */
int c_range_start;          /* node count of current range start */
int curr_obs_points;        /* current number of observation points */
int current_calls = 0;      /* call counter in match */
int current_call_lim = 0;   /* current call limit in match */
int current_const = 0;      /* current constraint number in match */
int edit_is_on = 0;         /* != 0 if inside current sequence edit */
int group_is_on = 0;        /* true when inside group */
int in_stop = 0;            /* input buffer stop flag */
int inbuf_level = 0;        /* input buffer level */
int init_warn = 1;          /* intialisation warning level */
int interactive;            /* non-zero if interactive */
int irn_rand[NR_RAND];      /* for random generator */
int keep_tw_print;          /* previous twiss print flag (match) */
int loop_cnt = 0;           /* used to detect infinite loops */
int match_calls = 0;        /* command call limit in match */
int match_is_on = 0;        /* true when inside match command */
int match_num_beta = 0;     /* OB 23.1.2002 */
int match_num_seqs = 0;     /* OB 23.1.2002 */
int mig_strategy;           /* migrad strategy (match) */
int new_name_count = 0;     /* to make internal names */
int next_rand = 0;          /* for random generator */
int plots_made = 0;         /* set to 1 if plots are made */
int polish_cnt = 0;         /* used to detect infinite loops */
int print_match_summary = 0;/* OB 6.3.2002:
			       activate the print option in the 
			       'mtgeti' and 'collect' routines */
int quote_toggle = 0;       /* for quote strings on input */
int scrap_count = 0;        /* running counter to make things unique */
int seqedit_install = 0;    /* counter for seqedit installs */
int seqedit_move = 0;       /* counter for seqedit moves */
int seqedit_remove = 0;     /* counter for seqedit removes */
int sequ_is_on = 0;         /* != 0 if inside current sequence decl. */
int start_cnt = 0;          /* counter for start commands */
int total_const = 0;        /* total no. of constraints in match */
int total_vars = 0;         /* total no. of variables in match */
int track_is_on = 0;        /* true when inside track command */
int track_start_cnt = 0;    /* counter for track start commands */
int twiss_success = 0;      /* set by twiss module to 1 if OK */
int use_count = 0;          /* incremented by 1 every time use is executed */
int vary_cnt = 0;           /* counter for vary commands */
int watch_flag = 0;         /* produces debug output when != 0 */
int stamp_flag = 0;         /* checks for double delete when != 0 */

time_t last_time;
time_t start_time;

/* end of definitions */
