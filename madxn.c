/* Production version of MAD-X, version number: see madxd.h */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#ifndef _WIN32
#include <sys/utsname.h>
#include <unistd.h>
#endif
#include <sys/timeb.h>
#include <ctype.h>
#include <math.h>
#include <time.h>
#ifdef _CATCH_MEM
#include <signal.h>
#endif

#include "c6t.c"

#include "madxe.c"

#include "madxc.c"

#ifdef _ONLINE
#include "madxsdds.c"
#endif

#include "sxf.c"

#include "makethin.c"

#include "matchc.c"
#include "matchc2.c"

#include "aperture.c"

#ifdef _WRAP_FORTRAN_CALLS
#include "fortran_wrappers.h"
#endif
#ifdef _WRAP_C_CALLS
#include "c_wrappers.h"
#endif

double** trackstrarpositions = 0x0;/* two dimensional array with track positions*/


void adjust_beam()
  /* adjusts beam parameters to current beta, gamma, bcurrent, npart */
{
  struct name_list* nl = current_beam->par_names;
  double circ = one, freq0, alfa, beta, gamma, bcurrent = zero, npart = 0;
  if (current_sequ != NULL && sequence_length(current_sequ) != zero)
    circ = current_sequ->length;
  beta = command_par_value("beta", current_beam);
  gamma = command_par_value("gamma", current_beam);
  alfa = one / (gamma * gamma);
  freq0 = (beta * clight) / (ten_p_6 * circ);
  if (nl->inform[name_list_pos("bcurrent", nl)] &&
      (bcurrent = command_par_value("bcurrent", current_beam)) > zero)
    npart = bcurrent / (freq0 * ten_p_6 * get_variable("qelect"));
  else if (nl->inform[name_list_pos("npart", nl)] &&
           (npart = command_par_value("npart", current_beam)) > zero)
    bcurrent = npart * freq0 * ten_p_6 * get_variable("qelect");
  store_comm_par_value("alfa", alfa, current_beam);
  store_comm_par_value("freq0", freq0, current_beam);
  store_comm_par_value("circ", circ, current_beam);
  store_comm_par_value("npart", npart, current_beam);
  store_comm_par_value("bcurrent", bcurrent, current_beam);
}

void adjust_probe(double delta_p)
  /* adjusts beam parameters to the current deltap */
{
  int j;
  double etas, slope, qs, fact, tmp, ds = oneturnmat[34];
  double alfa, beta, gamma, dtbyds, circ, deltat, freq0;
  double betas, gammas, et, sigt, sige;
  et = command_par_value("et", current_beam);
  sigt = command_par_value("sigt", current_beam);
  sige = command_par_value("sige", current_beam);
  beta = command_par_value("beta", current_beam);
  gamma = command_par_value("gamma", current_beam);
  circ = command_par_value("circ", current_beam);
  for (j = 0; j < 4; j++) ds += oneturnmat[4 + 6*j] * disp0[j];
  tmp = - beta * beta * ds / circ;
  freq0 = (clight * ten_m_6 * beta) / (circ * (one + tmp * delta_p));
  etas = beta * gamma * (one + delta_p);
  gammas = sqrt(one + etas * etas);
  betas = etas / gammas;
  tmp = - betas * betas * ds / circ;
  alfa = one / (gammas * gammas) + tmp;
  dtbyds = delta_p * tmp / betas;
  deltat = circ * dtbyds;
  store_comm_par_value("freq0", freq0, probe_beam);
  store_comm_par_value("alfa", alfa, probe_beam);
  store_comm_par_value("beta", betas, probe_beam);
  store_comm_par_value("gamma", gammas, probe_beam);
  store_comm_par_value("dtbyds", dtbyds, probe_beam);
  store_comm_par_value("deltap", delta_p, probe_beam);
  slope = -rfc_slope();
  qs = sqrt(fabs((tmp * slope) / (twopi * betas)));
  if (qs != zero)
  {
    fact = (tmp * circ) / (twopi * qs);
    if (et > zero)
    {
      sigt = sqrt(fabs(et * fact));
      sige = sqrt(fabs(et / fact));
    }
    else if (sigt > zero)
    {
      sige = sigt / fact;
      et = sige * sigt;
    }
    else if (sige > zero)
    {
      sigt = sige * fact;
      et = sige * sigt;
    }
  }
  if (sigt < ten_m_15)
  {
    put_info("Zero value of SIGT", "replaced by 1.");
    sigt = one;
  }
  if (sige < ten_m_15)
  {
    put_info("Zero value of SIGE", "replaced by 1/1000.");
    sigt = ten_m_3;
  }
  store_comm_par_value("qs", qs, probe_beam);
  store_comm_par_value("et", et, probe_beam);
  store_comm_par_value("sigt", sigt, probe_beam);
  store_comm_par_value("sige", sige, probe_beam);
}

void adjust_rfc()
{
  /* adjusts rfc frequency to given harmon number */
  double freq0, harmon, freq;
  int i;
  struct element* el;
  freq0 = command_par_value("freq0", probe_beam);
  for (i = 0; i < current_sequ->cavities->curr; i++)
  {
    el = current_sequ->cavities->elem[i];
    if ((harmon = command_par_value("harmon", el->def)) > zero)
    {
      freq = freq0 * harmon;
      store_comm_par_value("freq", freq, el->def);
    }
  }
}

int advance_node()
  /* advances to next node in expanded sequence;
     returns 0 if end of range, else 1 */
{
  if (current_node == current_sequ->range_end)  return 0;
  current_node = current_node->next;
  return 1;
}

int advance_to_pos(char* table, int* t_pos)
  /* advances current_node to node at t_pos in table */
{
  struct table* t;
  int pos, cnt = 0, ret = 0;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
  {
    ret = 1;
    t = table_register->tables[pos];
    if (t->origin == 1)  return 0; /* table is read, has no node pointers */
    while (current_node)
    {
      if (current_node == t->p_nodes[*t_pos-1]) break;
      if ((current_node = current_node->next)
          == current_sequ->ex_start) cnt++;
      if (cnt > 1) return 0;
    }
  }
  return ret;
}

/*moved at the top, bj 21 june 2007*/
/*#include "aperture.c"*/

int attach_beam(struct sequence* sequ)
  /* attaches the beam belonging to the current sequence */
{
  if ((current_beam = find_command(sequ->name, beam_list)) == NULL)
    current_beam = find_command("default_beam", beam_list);
  return current_beam->beam_def;
}

void augment_count(char* table) /* increase table occ. by 1, fill missing */
{
  int pos;
  struct table* t;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else {
    warning("Can not find table",table);
    return;
  }

  if (strcmp(t->type, "twiss") == 0) complete_twiss_table(t);

  if (t->num_cols > t->org_cols)  add_vars_to_table(t);

  if (t->p_nodes != NULL) t->p_nodes[t->curr] = current_node;

  if (t->node_nm != NULL)
  {
    t->node_nm->p[t->curr] = current_node->name;
    t->node_nm->curr = t->curr;
  }
  if (++t->curr == t->max) grow_table(t);
}

void augmentcountonly(char* table) /* increase table occ. by 1 */
{
  int pos;
  struct table* t;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else
  {
    warning("Can not find table",table);
    return;
  }

  if (t->num_cols > t->org_cols)  add_vars_to_table(t);

  if (++t->curr == t->max) grow_table(t);
}

char* buffer(char* string)  /* replaced by permbuff */
{
  return permbuff(string);
}

int char_from_table(char* table, char* name, int* row, char* val)
  /* OB 2.4.2002 */
  /* returns val at position row in column with name "name".
     function value return:
     0  OK
     -1 table  does not exist
     -2 column does not exist
     -3 row    does not exist
  */
{
  int pos;
  struct table* t;

  strcpy(val,"No-Name");
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return -1;
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) < 0) return -2;
  if (*row > t->curr)  return -3;
  strncpy(val,t->node_nm->p[*row-1],NAME_L);
  while (strlen(val)<=NAME_L) val[strlen(val)]=' ';
  return 0;
}

struct double_array* command_par_array(char* parameter, struct command* cmd)
  /* returns an updated command parameter array if found, else NULL */
{
  struct command_parameter* cp;
  struct double_array* arr = NULL;
  int i;
  if ((i = name_list_pos(parameter, cmd->par_names)) > -1)
  {
    cp = cmd->par->parameters[i];
    if (cp->type == 11 || cp->type == 12)
    {
      arr = cp->double_array;
      if (cp->expr_list != NULL) update_vector(cp->expr_list, arr);
    }
  }
  return arr;
}

int command_par_vector(char* parameter, struct command* cmd, double* vector)
  /* returns the length of, and an updated command parameter vector
     if found, else 0 */

{
  struct command_parameter* cp;
  int i;
  if ((i = name_list_pos(parameter, cmd->par_names)) > -1)
  {
    cp = cmd->par->parameters[i];
    if (cp->double_array != NULL)
    {
      if (cp->expr_list != NULL)
        update_vector(cp->expr_list, cp->double_array);
      copy_double(cp->double_array->a, vector, cp->double_array->curr);
      return cp->double_array->curr;
    }
  }
  return 0;
}

void comment_to_table(char* table, char* comment, int* length)
  /* Saves the comment string at the current line.
     This comment is then printed in front of this line.
     Several calls to the same current line are possible. */
{
  int pos;
  struct table* t;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return;
  strncpy(c_dum->c, comment, *length); c_dum->c[*length] = '\0';
  if (t->l_head[t->curr] == NULL)
    t->l_head[t->curr] = new_char_p_array(2);
  else if (t->l_head[t->curr]->curr == t->l_head[t->curr]->max)
    grow_char_p_array(t->l_head[t->curr]);
  t->l_head[t->curr]->p[t->l_head[t->curr]->curr++] = tmpbuff(c_dum->c);
}

void comm_para(char* name, int* n_int, int* n_double, int* n_string,
               int* int_array, double* double_array, char* strings,
               int* string_lengths)
  /* returns the value for command parameter "name" being either
     one or several integers (including logicals),
     one or several doubles,
     one or several strings (packed in one, with length array)
     Input:
     name                  parameter name
     Output:
     n_int                 # integers
     n_double              # double
     n_string              # strings
     int_array             array for integers
     double_array          array for doubles
     strings               one string for all, packed
     string_lengths        length of each string in char

     ATTENTION: no check on sufficient array sizes
  */
{
  int i, l, pos;
  struct command_parameter* cp;
  struct double_array* arr = NULL;
  *n_int = *n_double = *n_string = 0;
  mycpy(c_dum->c, name);
  if (this_cmd != NULL && this_cmd->clone != NULL)
  {
    if ((pos = name_list_pos(c_dum->c, this_cmd->clone->par_names)) > -1)
    {
      cp = this_cmd->clone->par->parameters[pos];
      switch (cp->type)
      {
        case 0:
          *n_int = 1;
          *int_array = cp->double_value;
          break;
        case 1:
          *n_int = 1;
          if (cp->expr == NULL) *int_array = cp->double_value;
          else *int_array = expression_value(cp->expr, 2);
          break;
        case 2:
          *n_double = 1;
          if (cp->expr == NULL) *double_array = cp->double_value;
          else *double_array = expression_value(cp->expr, 2);
          break;
        case 3:
          if (cp->string != NULL)
          {
            *n_string = 1;
            l = *string_lengths = strlen(cp->string);
            strncpy(strings, cp->string, l);
          }
          break;
        case 11:
        case 12:
          arr = cp->double_array;
          if (cp->expr_list != NULL) update_vector(cp->expr_list, arr);
          if (cp->type == 11)
          {
            for (i = 0; i < arr->curr; i++) int_array[i] = arr->a[i];
            *n_int = arr->curr;
          }
          else
          {
            for (i = 0; i < arr->curr; i++) double_array[i] = arr->a[i];
            *n_double = arr->curr;
          }
          break;
        case 13:
          for (i = 0; i < cp->m_string->curr; i++)
          {
            string_lengths[i] = l = strlen(cp->m_string->p[i]);
            strncpy(strings, cp->m_string->p[i], l);
            strings += l;
          }
          *n_string = cp->m_string->curr;
      }
    }
  }
}

void complete_twiss_table(struct table* t)
  /* fills all items missing after "twiss" into twiss table */
{
  int i, j, mult, n, myrbend;
  double el, val;
  struct node* c_node;
  char tmp[16];

  if (t == NULL) return;
  i = t->curr;
  c_node = current_node;
  mult = strcmp(c_node->base_name, "multipole") == 0 ? 1 : 0;
  t->s_cols[0][i] = tmpbuff(c_node->name);
  t->s_cols[1][i] = tmpbuff(c_node->base_name);
  t->s_cols[twiss_fill_end+1][i] = tmpbuff(c_node->p_elem->parent->name);
  for (j = twiss_opt_end+1; j<= twiss_fill_end; j++)
  {
    el = c_node->length;
    strcpy(tmp, twiss_table_cols[j]);
    myrbend = (strcmp(c_node->p_elem->base_type->name, "rbend") == 0);
    if (strcmp(twiss_table_cols[j], "l") == 0) val = el;
    else if (strcmp(tmp, "slot_id") == 0) val =  el_par_value(tmp, c_node->p_elem);
    else if (strcmp(tmp, "e1") == 0 || strcmp(tmp, "e2") == 0)
    {
      if(myrbend)
      {
        val =  el_par_value(tmp, c_node->p_elem) +
          c_node->other_bv * el_par_value("angle", c_node->p_elem) / two;
      }
      else
      {
        val =  el_par_value(tmp, c_node->p_elem);
      }
    }
    else if (strcmp(tmp, "assembly_id") == 0) val =  el_par_value(tmp, c_node->p_elem);
    else if (strcmp(tmp, "mech_sep") == 0) val =  el_par_value(tmp, c_node->p_elem);
    else if (strcmp(tmp, "lrad") == 0) val =  el_par_value(tmp, c_node->p_elem);
    else if(mult)
    {
      if(j<=twiss_mult_end)
      {
        val = mult_par(twiss_table_cols[j], c_node->p_elem);
        val *= c_node->other_bv; /* dipole_bv kill initiative SF TR FS */
      }
      else
      {
        val = el_par_value(tmp, c_node->p_elem);
      }
    }
    else
    {
      strcpy(tmp, twiss_table_cols[j]);
      n = strlen(tmp) - 1;
      if (n > 1 && tmp[0] == 'k' && isdigit(tmp[1]) && tmp[n] == 'l')
        tmp[n] = '\0'; /* suppress trailing l in k0l etc. */
      if (el != zero && n > 1 && tmp[0] == 'k' && tmp[1] == 's' && tmp[n] == 'i')
        tmp[n] = '\0'; /* suppress trailing i in ksi */
      val = el_par_value(tmp, c_node->p_elem);
      if (n > 1 && tmp[0] == 'k' && isdigit(tmp[1]))
        val *= c_node->other_bv; /* dipole_bv kill initiative SF TR FS */
      else if (strstr(tmp, "kick") || strcmp(tmp, "angle") == 0 ||
               strcmp(tmp, "ks") == 0 || strcmp(tmp, "ksi") == 0 ||
               strcmp(tmp, "volt") == 0 )
        val *= c_node->other_bv; /* dipole_bv kill initiative SF TR FS */
      if (el != zero)
      {
        if (strstr(tmp,"kick") == NULL && strcmp(tmp, "angle")
            && strcmp(tmp, "tilt") && strcmp(tmp, "e1") && strcmp(tmp, "e2")
            && strcmp(tmp, "h1") && strcmp(tmp, "h2") && strcmp(tmp, "hgap")
            && strcmp(tmp, "fint") && strcmp(tmp, "fintx")
            && strcmp(tmp, "volt") && strcmp(tmp, "lag")
            && strcmp(tmp, "freq") && strcmp(tmp, "harmon")) val *= el;
      }
    }
    t->d_cols[j][i] = val;
  }
}

double double_from_expr(char** toks, int s_start, int s_end)
  /* returns the value of an expression if valid, else INVALID */
{
  int end, nitem = s_end + 1;
  int type = loc_expr(toks, nitem, s_start, &end);
  if (type == 1) /* simple number */
    return simple_double(toks, s_start, end);
  else if (polish_expr(end + 1 - s_start, &toks[s_start]) == 0)
    return polish_value(deco, join( &toks[s_start],end + 1 - s_start) );
  else return INVALID;
}

void double_to_table(char* table, char* name, double* val)
  /* puts val at current position in column with name "name".
     The table count is increased separately with "augment_count" */
{
  int pos;
  struct table* t;

  /*  printf("double_to_table <%s> <%s> <%f>\n",table,name,*val);*/

  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
  {
    t = table_register->tables[pos];
  }
  else
  {
    printf("Can not find table %s\n",table);
    return;
  }
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) >= 0 && t->columns->inform[pos] < 3)
  {
    t->d_cols[pos][t->curr] = *val;
  }
  else
  {
    printf("Position of column %s is %d\n",name,pos);
  }
}

void double_to_table_row(char* table, char* name, int* row, double* val)
  /* puts val at row position in column with name "name".
     The table count is increased separately with "augment_count" */
{
  int pos;
  struct table* t;

  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return;
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) >= 0
      && t->columns->inform[pos] < 3) t->d_cols[pos][*row-1] = *val;
}

void string_to_table_row(char* table, char* name, int* row, char* string)
  /* puts string at current position in column with name "name".
     The table count is increased separately with "augment_count" */
{
  int pos;
  struct table* t;

  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return;
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) >= 0
      && t->columns->inform[pos] == 3)
  {
    mycpy(c_dum->c, string);
    t->s_cols[pos][*row-1] = tmpbuff(c_dum->c);
  }
}

int double_from_table(char* table, char* name, int* row, double* val)
  /* returns val at position row in column with name "name".
     function value return:
     0  OK
     -1 table  does not exist
     -2 column does not exist
     -3 row    does not exist
  */
{
  int pos;
  struct table* t;

  *val = zero;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return -1;
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) < 0) return -2;
  if (*row > t->curr)  return -3;
  *val = t->d_cols[pos][*row-1];
  return 0;
}

int string_from_table(char* table, char* name, int* row, char* string)
  /* returns val at position row in column with name "name".
     function value return:
     0  OK
     -1 table  does not exist
     -2 column does not exist
     -3 row    does not exist
     struct command_parameter* cp;
     struct double_array* arr = NULL;
  */
{
  int pos,l;
  struct table* t;

  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return -1;
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) < 0) return -2;
  if (*row > t->curr)  return -3;
  l = strlen(t->s_cols[pos][*row-1]);
  mycpy(string, t->s_cols[pos][*row-1]);
  return 0;
}

int result_from_normal(char* name_var, int* order, double* val)
  /* returns value of table normal_results corresponding to the given variable name
     and to the given orders
     function value return:
     0  OK
     -1 table  does not exist
     -2 column does not exist
     -3 row    does not exist
  */
{
  int row,k,found,pos;
  char string[AUX_LG],n_var[AUX_LG];
  double d_val=zero;
  struct table* t;

  pos = name_list_pos("normal_results", table_register->names);
  t = table_register->tables[pos];

  *val = zero;
  found = 0;
  mycpy(n_var, name_var);
  for (row = 1; row <= t->curr; row++)
  {
    k = string_from_table("normal_results","name", &row, string);
    if (k != 0) return k;
    if (strcmp(string,n_var) == 0)
    {
      found = 1;
      k = double_from_table("normal_results","order1", &row, &d_val);
      if ((int)d_val != order[0]) found = 0;
      k = double_from_table("normal_results","order2", &row, &d_val);
      if ((int)d_val != order[1]) found = 0;
      k = double_from_table("normal_results","order3", &row, &d_val);
      if ((int)d_val != order[2]) found = 0;
      k = double_from_table("normal_results","order4", &row, &d_val);
      if ((int)d_val != order[3]) found = 0;
    }
    if (found == 1) break;
  }
  if (found == 1)
    k = double_from_table("normal_results","value", &row, &d_val);
  *val = d_val;
  return 0;
}

void dynap_tables_create(struct in_cmd* cmd)
  /* creates the dynamic tables for DYNAP execution */
{
  int npart = stored_track_start->curr;

  struct table* t;
  t = make_table("tracksumm", "tracksumm", tracksumm_table_cols,
                 tracksumm_table_types, 2*stored_track_start->curr);
  add_to_table_list(t, table_register);
  t = make_table("dynap", "dynap", dynap_table_cols, dynap_table_types, 10);
  add_to_table_list(t, table_register);
  t = make_table("dynaptune", "dynaptune", dynaptune_table_cols,
                 dynaptune_table_types, npart);
  add_to_table_list(t, table_register);
}

void element_name(char* name, int* l)
  /* returns current node element name in Fortran format */
  /* l is max. allowed length in name */
{
  int ename_l = strlen(current_node->p_elem->name);
  int i, ncp = ename_l < *l ? ename_l : *l;
  int nbl = *l - ncp;
  for (i = 0; i < ncp; i++) name[i] = current_node->p_elem->name[i];
  for (i = 0; i < nbl; i++) name[ncp+i] = ' ';
}

void node_name(char* name, int* l)
  /* returns current node name in Fortran format */
  /* l is max. allowed length in name */
{
  int ename_l = strlen(current_node->name);
  int i, ncp = ename_l < *l ? ename_l : *l;
  int nbl = *l - ncp;
  for (i = 0; i < ncp; i++) name[i] = toupper(current_node->name[i]);
  for (i = 0; i < nbl; i++) name[ncp+i] = ' ';
}


int embedded_plot()
  /* returns the embedded_flag */
{
  int ret;
  ret = embedded_flag;
  return ret;
}

void exec_create_table(struct in_cmd* cmd)
  /* makes a user defined table */
{
  char rout_name[] = "exec_create_table";
  struct table* t;
  int* t_types;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct char_p_array* m;
  char** t_c;
  int j, pos = name_list_pos("table", nl);
  char* name = NULL;
  int  ncols = 0;  /*number of columns*/

  if (nl->inform[pos] == 0)
  {
    warning("no table name:", "ignored");
    return;
  }
  if ((name = pl->parameters[pos]->string) == NULL)
  {
    warning("no table name: ", "ignored");
    return;
  }
  if ((pos = name_list_pos(name, table_register->names)) > -1)
  {
    warning("table already exists: ", "ignored");
    return;
  }

  pos = name_list_pos("column", nl);
  if (nl->inform[pos] == 0)
  {
    warning("table without columns: ", "ignored");
    return;
  }
  m = pl->parameters[pos]->m_string;
  ncols = m->curr;
  /* now make table */
  t_types = mymalloc(rout_name, ncols*sizeof(int));
  t_c = mymalloc(rout_name, (ncols+1)*sizeof(char*));

  for (j = 0; j < m->curr; j++)
  {
    if (*m->p[j] == '_')
    {
      t_types[j] = 3; /* type string */
      t_c[j] = permbuff(&m->p[j][1]);
    }
    else
    {
      t_types[j] = 2; /* type double */
      t_c[j] = permbuff(m->p[j]);
    }
  }
  t_c[ncols] = blank;
  t = make_table(name, "user", t_c, t_types, USER_TABLE_LENGTH);
  t->org_cols = 0;  /* all entries are "added" */
  add_to_table_list(t, table_register);
  myfree(rout_name, t_c); myfree(rout_name, t_types);
  t->dynamic = 1;
}

void exec_store_coguess(struct in_cmd* cmd)
  /* stores the initial orbit guess of the user */
{
  struct name_list* nl = cmd->clone->par_names;
  int pos = name_list_pos("tolerance", nl);
  double tol;
  if (nl->inform[pos])
  {
    tol = command_par_value("tolerance", cmd->clone);
    set_variable("twiss_tol", &tol);
  }
  store_orbit(cmd->clone, guess_orbit);
  guess_flag = 1;
}

void exec_dump(struct in_cmd* cmd)
  /* write a table out */
{
  struct table* t;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("table", nl);
  char* name = NULL;
  char *f, filename[FNAME_L];
  if (nl->inform[pos] == 0)
  {
    warning("dump without table name:", "ignored");
    return;
  }
  if ((name = pl->parameters[pos]->string) == NULL)
  {
    warning("dump without table name:", "ignored");
    return;
  }
  pos = name_list_pos("file", nl);
  if (nl->inform[pos] == 0) strcpy(filename, "terminal");
  else if ((f = pl->parameters[pos]->string) == NULL
           || *f == '\0') strcpy(filename, name);
  else strcpy(filename,f);
  if ((pos = name_list_pos(name, table_register->names)) > -1)
  {
    t = table_register->tables[pos];
    out_table(name, t, filename);
  }
  else
  {
    warning("table name not found:", "ignored");
  }
}

void exec_fill_table(struct in_cmd* cmd)
  /* adds variables to a table */
{
  struct table* t;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("table", nl);
  char* name = NULL;
  int row,curr;
  if (nl->inform[pos] == 0)
  {
    warning("no table name:", "ignored");
    return;
  }
  if ((name = pl->parameters[pos]->string) == NULL)
  {
    warning("no table name: ", "ignored");
    return;
  }
  pos=name_list_pos("row", nl);
  row=(int) pl->parameters[pos]->double_value;
  if ((pos = name_list_pos(name, table_register->names)) > -1)
  {
    t = table_register->tables[pos];
    if (row<0) {
      add_vars_to_table(t);
      if (++t->curr == t->max) grow_table(t);
    } else {
      row--;
      curr=t->curr;
      if (row < t->curr) {
        t->curr=row;}
      else {
        t->curr--;
      }
      add_vars_to_table(t);
      t->curr=curr;
    }
  }
  else warning("table not found: ", "ignored");
  return;
}

void exec_setvars_table(struct in_cmd* cmd)
  /* set variables from a table */
{
  struct table* t;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("table", nl);
  char* name = NULL;
  int row,curr;
  if (nl->inform[pos] == 0)
  {
    warning("no table name:", "ignored");
    return;
  }
  if ((name = pl->parameters[pos]->string) == NULL)
  {
    warning("no table name: ", "ignored");
    return;
  }
  current_node = NULL; /* to distinguish from other table fills */
  pos=name_list_pos("row", nl);
  row=(int) pl->parameters[pos]->double_value;
  if ((pos = name_list_pos(name, table_register->names)) > -1)
  {
    t = table_register->tables[pos];
    row--;
    curr=t->curr;
    if ((row < t->curr) && (row >-1)) {
      t->curr=row;}
    else {
      t->curr--;
    }
    set_vars_from_table(t);
    t->curr=curr;
  }
  else warning("table not found: ", "ignored");
  return;
}


/* Append the gnuplot ps file to the main ps file */
void gnuplot_append(char *gplfilename, char *psfilename){
  char line[1000];
  FILE *newpsfile;
  FILE *oldpsfile;
  FILE *gplpsfile;
  int np=0;
  int page=0;
  /* if psfilename does not exist rename it as psfilename and exit*/
  newpsfile=fopen(psfilename,"r");
  if( newpsfile==NULL) {
    rename(gplfilename,psfilename);
    return;
  }
  else
  {
    /* the file has to be closed it is going to change */
    fclose(newpsfile);
  };

  /* else append the gnuplot ps file to psfilename
     Save old value */
  rename(psfilename,"tmpoldplot.ps");
  newpsfile=fopen(psfilename,"w");
  oldpsfile=fopen("tmpoldplot.ps","r");
  gplpsfile=fopen(gplfilename,"r");
  /* read old ps file and copy on the new ps file */
  while(fgets(line,1000,oldpsfile)!=NULL){
    /* don't print after %%Trailer */
    if (strncmp("%%Trailer",line,9)==0)  np=1;
    /* Count the pages and rewrite the line */
    if (strncmp("%%Page:",line,7)==0) {
      page++;
      fprintf(newpsfile,"%%%%Page: %d %d\n",page,page);
    }
    else
    {
      /* write the lines */
      if(np==0) {
        fprintf(newpsfile,"%s",line);
      }
    }
  }
  fclose(oldpsfile);
  remove("tmpoldplot.ps");
  /* read gnuplot ps file and append on the final file */
  while(fgets(line,1000,gplpsfile)!=NULL){
    /* don't print after %%Trailer */
    if (strncmp("%%Trailer",line,9)==0)  np=1;
    /* Count the pages and rewrite the line */
    if (strncmp("%%Page:",line,7)==0) {
      page++;
      fprintf(newpsfile,"%%%%Page: %d %d\n",page,page);
    }
    else
    {
      if(np==0) {
        /* write the lines */
        fprintf(newpsfile,"%s",line);
      }
    }
    /* Print after prologue */
    if (strncmp("%%EndProlog",line,11)==0) np=0;
  }
  fclose(gplpsfile);
  remove("tmpplot.ps");
  /* Print the trailer */
  fprintf(newpsfile,"%%%%Trailer\n");
  fprintf(newpsfile,"%%%%DocumentFonts: Times-Roman\n");
  fprintf(newpsfile,"%%%%Pages: %d\n",page);
  fprintf(newpsfile,"%%%%EOF\n");
  fclose(newpsfile);
}



void exec_plot(struct in_cmd* cmd)
{
  int i, j, k, ierr, pos, nt = strcmp(title,"no-title") == 0 ? 1 : 0;
  int nointerp = 0, multiple = 0, noversion = 0, nolegend = 0, s_haxis = 1, track_flag = 0;
  int tsm1 = TITLE_SIZE - 1, tsm2 = TITLE_SIZE - 2;
  int part_idx[100], curr, track_cols_length, haxis_idx = 0, vaxis_idx = 0;
  int size_plot_title = tsm1, size_version = tsm1;
  int *title_length = &size_plot_title, *version_length = &size_version;
  char* pt = title, *haxis_name = NULL, *vaxis_name = NULL, *file_name = NULL;
  char* particle_list;
  struct name_list* nl_plot = NULL;
  struct command_parameter_list* pl_plot = NULL;
  char *table_name, *last_twiss_table, *trackfile;
  char track_file_name[NAME_L], ps_file_name[NAME_L];
  char plot_title[TITLE_SIZE], version[TITLE_SIZE];
  FILE *gpu;

  /* use correct beam for sequence to be plotted - HG 031127 */
  struct command* keep_beam = current_beam;
  if (attach_beam(current_sequ) == 0)
    fatal_error("TWISS - sequence without beam:", current_sequ->name);
  /* end part1 of HG 031127 */

  /* Check table name is the same as in the last twiss command */
  if (this_cmd != NULL && this_cmd->clone != NULL)
  {
    nl_plot = this_cmd->clone->par_names;
    pl_plot = this_cmd->clone->par;

    /* get vaxis_name */

    pos = name_list_pos("vaxis", nl_plot);

    vaxis_name = pl_plot->parameters[pos]->m_string->p[0];

    /* get interpolation */

    pos = name_list_pos("interpolation", nl_plot);
    nointerp = 1 - nl_plot->inform[pos];

    /* get haxis_name & s_haxis flag */

    pos = name_list_pos("haxis", nl_plot);
    if(nl_plot->inform[pos])
    {
      if ((haxis_name = pl_plot->parameters[pos]->string) == NULL)
        haxis_name = pl_plot->parameters[pos]->call_def->string;
      s_haxis = strcmp(haxis_name,"s");
    }

    /* get table_name & track_flag */

    pos = name_list_pos("table", nl_plot);
    if(nl_plot->inform[pos]) /* table name specified */
    {

      if ((table_name = pl_plot->parameters[pos]->string) == NULL)
        table_name = pl_plot->parameters[pos]->call_def->string;

      if(strcmp(table_name,"track") == 0)
        track_flag = 1;
    }
    else
      table_name = "twiss";

    /* check if table name is the same of the last twiss call if haxis is "s" and no interpolation */

    if(nointerp == 0 && s_haxis == 0)
    {
      last_twiss_table = current_sequ->tw_table->name;
      if (strcmp(table_name,"aperture") != 0 )
      {
        if(strcmp(table_name,last_twiss_table) != 0)
        {
          printf("Only allowed table attribute in plot command is \"aperture\". Else, table name is automatically changed to %s \n",last_twiss_table );
          if ((pl_plot->parameters[pos]->string = last_twiss_table) == NULL)
            pl_plot->parameters[pos]->call_def->string =last_twiss_table ;
        }
      }
    }

    /* get file_name */

    pos = name_list_pos("file", nl_plot);
    if(nl_plot->inform[pos]) /* file name specified */
    {
      if ((file_name = pl_plot->parameters[pos]->string) == NULL)
        file_name = pl_plot->parameters[pos]->call_def->string;
    }
    else
    {
      if (track_flag)
        file_name = "madx_track";
      else
        file_name = "madx";
    }
  }
  else
    fatal_error("Plot "," - non existing command");

  /* If table name is "track" use the gnuplot package */

  if (track_flag)
  {
    /* get track file name */

    trackfile = command_par_string("trackfile", this_cmd->clone);

    /* get particle */

    pos = name_list_pos("particle", nl_plot);
    curr = pl_plot->parameters[pos]->m_string->curr;
    for (i = 0; i < curr; i++)
    {
      particle_list = pl_plot->parameters[pos]->m_string->p[i];
      part_idx[i] = atoi(particle_list);
    }

    /* get multiple */

    pos = name_list_pos("multiple", nl_plot);
    multiple = nl_plot->inform[pos];

    /* get noversion */

    pos = name_list_pos("noversion", nl_plot);
    noversion = nl_plot->inform[pos];

    /* get nolegend */

    pos = name_list_pos("nolegend", nl_plot);
    nolegend = nl_plot->inform[pos];

    /* find the column numbers corresponding to haxis_name & vaxis_name */

    track_cols_length = sizeof(track_table_cols)/4 - 1;
    for (j = 0; j < track_cols_length; j++)
    {
      if(strcmp(track_table_cols[j],haxis_name) == 0 && haxis_idx == 0)
        haxis_idx = j + 1;
      if(strcmp(track_table_cols[j],vaxis_name) == 0 && vaxis_idx == 0)
        vaxis_idx = j + 1;
    }

    /* build-up the title */

    for (j = 0; j < tsm1; j++)
    {
      plot_title[j] = ' ';
      version[j] = ' ';
    }
    plot_title[tsm1] = '\0';
    version[tsm1] = '\0';
    get_title(plot_title,title_length);
    for (k = *title_length + 1; k > 0; k--)
    {
      plot_title[k] = plot_title[k - 1];
    }
    plot_title[0]= '\"';
    if (noversion)
    {
      plot_title[*title_length+1] =  '\"';
      plot_title[*title_length+2] =  '\0';
    }
    else
    {
      plot_title[tsm2] =  '\"';
      get_version(version,version_length);
      k = tsm2 - *version_length;
      for (j = k; j < tsm2; j +=1)
      {
        plot_title[j] = version[j - k];
      }
    }

    /* build-up the gnuplot command file */
    mycpy(track_plot_filename,file_name);
    sprintf(ps_file_name,track_plot_filename);
    strcat(ps_file_name,".ps");

    gpu = fopen("gnu_plot.cmd","w");
    fprintf(gpu,"set terminal postscript color\n");
    fprintf(gpu,"set pointsize 0.48\n");
    fprintf(gpu,"set output '%s'\n","tmpplot.ps");

    fprintf(gpu,"set title %s\n",plot_title);
    fprintf(gpu,"set xlabel '%s'\n",haxis_name);
    fprintf(gpu,"set ylabel '%s'\n",vaxis_name);
    for (j = 0; j < curr; j++)
    {
      sprintf(track_file_name, "%s.obs%04d.p%04d", trackfile, 1, part_idx[j]);
      if (fopen(track_file_name,"r") == NULL)
        printf("file %s does not exist \n",track_file_name);
      else
      {
        if (j == 0) fprintf(gpu,"plot ");
        else
        {
          if (multiple == 0)
            fprintf(gpu,"\nplot ");
          else
            fprintf(gpu,", \\\n     ");
        }
        fprintf(gpu,"'%s' using %d:%d ",track_file_name,haxis_idx,vaxis_idx);
        if (nolegend)
          fprintf(gpu,"notitle with points %d ",part_idx[j]);
        else
          fprintf(gpu,"title 'particle %d' with points %d ",part_idx[j],part_idx[j]);

      }
    }
    fclose(gpu);
    /* gnuplot command file ready. it produces the file "tmpplot.ps"*/
    system("gnuplot 'gnu_plot.cmd'");
    /* Copy or append the gnuplot ps file in the target ps_file */
    gnuplot_append("tmpplot.ps",ps_file_name);
    /* Remove the gnuplot command */
/*    remove("gnu_plot.cmd");*/
  }
  else

    /* normal plot */

  {
    embedded_twiss_cmd = cmd;

    if (nt && current_sequ != NULL) title = current_sequ->name;
    pesopt_(&ierr);
    if (ierr == 0)
    {
      adjust_beam();
      probe_beam = clone_command(current_beam);
      adjust_probe(twiss_deltas->a[0]); /* sets correct gamma, beta, etc. */
      adjust_rfc(); /* sets freq in rf-cavities from probe */
      pefill_(&ierr);
      pemima_();
      plotit_(&plots_made);
      plots_made = 1;
    }
    if (nt) title = pt;
  }

  /* part 2 of HG 031127 */
  current_beam = keep_beam;
  /* end of part 2 of HG 031127 */
}

void exec_print(struct in_cmd* cmd)
  /* prints text from "print" command to current output unit */
{
  struct command_parameter_list* pl = cmd->clone->par;
  struct name_list* nl = cmd->clone->par_names;
  int pos = name_list_pos("text", nl);
  if (nl->inform[pos]) fprintf(prt_file,"%s\n", pl->parameters[pos]->string);
}

void exec_savebeta()
  /* stores twiss values in a beta0 structure */
{
  struct name_list* nl;
  struct command_parameter_list* pl;
  struct node* nodes[2];
  struct command* beta0;
  char* label;
  int i, pos;
  for (i = 0; i < savebeta_list->curr; i++)
  {
    nl = savebeta_list->commands[i]->par_names;
    pl = savebeta_list->commands[i]->par;
    pos = name_list_pos("label", nl);
    label = pl->parameters[pos]->string;
    if (find_command(label, beta0_list) == NULL) /* fill only once */
    {
      pos = name_list_pos("sequence", nl);
      if (nl->inform[pos] == 0
          || strcmp(pl->parameters[pos]->string, current_sequ->name) == 0)
      {
        pos = name_list_pos("place", nl);
        if (get_ex_range(pl->parameters[pos]->string, current_sequ, nodes))
        {
          pos = name_list_pos("beta0", defined_commands->list);
          beta0 = clone_command(defined_commands->commands[pos]);
          fill_beta0(beta0, nodes[0]);
          add_to_command_list(label, beta0, beta0_list, 0);
        }
      }
    }
  }
}

void exec_sodd(struct in_cmd* cmd)
{
  struct name_list* nl_sodd = NULL;
  int ierr,pos,nosixtrack;

  /* use correct beam for sequence to be plotted - HG 031127 */
  struct command* keep_beam = current_beam;


  if (attach_beam(current_sequ) == 0)
    fatal_error("TWISS - sequence without beam:", current_sequ->name);
  /* end part1 of HG 031127 */

  /* <JMJ 7/11/2002> The following ifndef exclusion is a quick fix so that
     the WIN32 version
     does not try to do X11 graphics. However this has the consequence that
     the program will not make Postscript files.  HG needs to separate these things.
     </JMJ 7/11/2002> */
  /*FS 27.03.2004 works now on Windows using gxx11ps.F and gxx11psc.c courtesy HG */

  /* get nosixtrack */

  if (this_cmd != NULL && this_cmd->clone != NULL)
  {
    nl_sodd = this_cmd->clone->par_names;
  }
  else
    fatal_error("SODD "," - No existing command");

  pos = name_list_pos("nosixtrack", nl_sodd);
  nosixtrack = nl_sodd->inform[pos];
  if(nosixtrack == 0)
  {
    printf("Build-up of input file fc.34 by call to program sixtrack. \n");
    conv_sixtrack(cmd);
    fclose(f34);
    printf("input file fc.34 is ready. \n");
  }
  sodd_table_70 = make_table("detune_1_end", "sodd_detune_5", sodd_detune_5_cols,
                             sodd_detune_5_types, 2);
  sodd_table_70->dynamic = 1;
  add_to_table_list(sodd_table_70, table_register);
  sodd_table_71 = make_table("detune_1_all", "sodd_detune_5", sodd_detune_5_cols,
                             sodd_detune_5_types, 2);
  sodd_table_71->dynamic = 1;
  add_to_table_list(sodd_table_71, table_register);
  sodd_table_72 = make_table("detune_2_hor", "sodd_detune_5", sodd_detune_5_cols,
                             sodd_detune_5_types, 2);
  sodd_table_72->dynamic = 1;
  add_to_table_list(sodd_table_72, table_register);
  sodd_table_73 = make_table("detune_2_ver", "sodd_detune_5", sodd_detune_5_cols,
                             sodd_detune_5_types, 2);
  sodd_table_73->dynamic = 1;
  add_to_table_list(sodd_table_73, table_register);
  sodd_table_74 = make_table("distort_1_f_end", "sodd_distort1_8", sodd_distort1_8_cols,
                             sodd_distort1_8_types, 2);
  sodd_table_74->dynamic = 1;
  add_to_table_list(sodd_table_74, table_register);
  sodd_table_75 = make_table("distort_1_h_end", "sodd_distort1_8", sodd_distort1_8_cols,
                             sodd_distort1_8_types, 2);
  sodd_table_75->dynamic = 1;
  add_to_table_list(sodd_table_75, table_register);
  sodd_table_76 = make_table("distort_1_f_all", "sodd_distort1_11", sodd_distort1_11_cols,
                             sodd_distort1_11_types, 2);
  sodd_table_76->dynamic = 1;
  add_to_table_list(sodd_table_76, table_register);
  sodd_table_77 = make_table("distort_1_h_all", "sodd_distort1_11", sodd_distort1_11_cols,
                             sodd_distort1_11_types, 2);
  sodd_table_77->dynamic = 1;
  add_to_table_list(sodd_table_77, table_register);
  sodd_table_78 = make_table("distort_2_f_end", "sodd_distort2_9", sodd_distort2_9_cols,
                             sodd_distort2_9_types, 2);
  sodd_table_78->dynamic = 1;
  add_to_table_list(sodd_table_78, table_register);
  sodd_table_79 = make_table("distort_2_h_end", "sodd_distort2_9", sodd_distort2_9_cols,
                             sodd_distort2_9_types, 2);
  sodd_table_79->dynamic = 1;
  add_to_table_list(sodd_table_79, table_register);
  soddin_(&ierr);

  /* part 2 of HG 031127 */
  current_beam = keep_beam;
  /* end of part 2 of HG 031127 */
}

void make_map_table(int* map_table_max_rows)
{
  int k, pos;
  if ((pos = name_list_pos("map_table", table_register->names)) > -1)
  {
    delete_table(table_register->tables[pos]);
    k = remove_from_name_list(table_register->tables[pos]->name,
                              table_register->names);
    table_register->tables[k] = table_register->tables[--table_register->curr];
  }
  /* initialise table */
  map_table = make_table("map_table", "map_tab", map_tab_cols,
                         map_tab_types, *map_table_max_rows);
  add_to_table_list(map_table, table_register);
  map_table->dynamic = 1;
  reset_count("map_table");
}

void select_ptc_normal(struct in_cmd* cmd)
  /* sets up all columns of the table normal_results except the last one (value) */
{
  struct name_list* nl;
  struct command_parameter_list* pl;
  struct table* t;
  int pos;
  int i, j, jj, curr;
  int skew, mynorder,myn1,myn2,mynres,indexa[4][1000];
  char* order_list;
  int min_req_order;
  double order[4],n1,n2,n3,n4;

  nl = this_cmd->clone->par_names;
  pl = this_cmd->clone->par;
  if (log_val("clear", cmd->clone))
  {
    min_order = 1;
    min_req_order = 1;
    mynres = 0;
    skew = 0;
    reset_count("normal_results");
/*    if ((pos = name_list_pos("normal_results", table_register->names)) > -1) delete_table(table_register->tables[pos]);*/
    return;
  }
  if ((pos = name_list_pos("normal_results", table_register->names)) <= -1)
  {
    /* initialise table */
    normal_results = make_table("normal_results", "normal_res", normal_res_cols,
                                normal_res_types, MAX_ROWS);
    normal_results->dynamic = 1;
    add_to_table_list(normal_results, table_register);
    reset_count("normal_results");
    pos = name_list_pos("normal_results", table_register->names);
    min_order = 1;
    min_req_order = 1;
  }
  t = table_register->tables[pos];

  /* initialise order array */
  order[0] = zero;
  order[1] = zero;
  order[2] = zero;
  order[3] = zero;
  if (t->curr == t->max) grow_table(t);

  for (j = 0; j < PTC_NAMES_L; j++)
  {
    /* Treat each ptc variable */

    pos = name_list_pos(names[j], nl);
    if (pos > -1 && nl->inform[pos])
    {
      curr = pl->parameters[pos]->m_string->curr;
      if (curr > 4)
        printf("Too many values for the attribute %s. Only the first four are retained.\n",names[j]);
      for (i = 0; i < curr; i++)
      {
        order_list = pl->parameters[pos]->m_string->p[i];
        order[i] = (double)atoi(order_list);
      }

      if (j == 10 || j == 11)
      {
        min_req_order = order[0]+order[1]+order[2];
        mynres = 0;
        skew = 0;
        mynorder = (int)order[0];
        if (mynorder < 0) skew = 1;
        mynorder = abs(mynorder);
        myn1 = (int)order[1];
        myn2 = (int)order[2];
        min_req_order = mynorder;
        res_index_(&skew, &mynorder, &myn1, &myn2, indexa, &mynres);
        if (mynres > 0)
        {
          if (j == 10)
          {
            for (jj = 0; jj < mynres; jj++)
            {
              n1 = (double)indexa[0][jj];
              n2 = (double)indexa[1][jj];
              n3 = (double)indexa[2][jj];
              n4 = (double)indexa[3][jj];
              string_to_table("normal_results", "name", "hamc");
              double_to_table("normal_results", "order1", &n1);
              double_to_table("normal_results", "order2", &n2);
              double_to_table("normal_results", "order3", &n3);
              double_to_table("normal_results", "order4", &n4);
              augment_count("normal_results");
              string_to_table("normal_results", "name", "hams");
              double_to_table("normal_results", "order1", &n1);
              double_to_table("normal_results", "order2", &n2);
              double_to_table("normal_results", "order3", &n3);
              double_to_table("normal_results", "order4", &n4);
              augment_count("normal_results");
              string_to_table("normal_results", "name", "hama");
              double_to_table("normal_results", "order1", &n1);
              double_to_table("normal_results", "order2", &n2);
              double_to_table("normal_results", "order3", &n3);
              double_to_table("normal_results", "order4", &n4);
              augment_count("normal_results");
            }
            string_to_table("normal_results", "name", "haml");
            double_to_table("normal_results", "order1", &order[0]);
            double_to_table("normal_results", "order2", &order[1]);
            double_to_table("normal_results", "order3", &order[2]);
            double_to_table("normal_results", "order4", &order[3]);
            n1 = (double)mynres;
            double_to_table("normal_results", "value", &n1);
            augment_count("normal_results");
          }
          if (j == 11)
          {
            for (jj = 0; jj < mynres; jj++)
            {
              n1 = (double)indexa[0][jj];
              n2 = (double)indexa[1][jj];
              n3 = (double)indexa[2][jj];
              n4 = (double)indexa[3][jj];
              string_to_table("normal_results", "name", "gnfc");
              double_to_table("normal_results", "order1", &n1);
              double_to_table("normal_results", "order2", &n2);
              double_to_table("normal_results", "order3", &n3);
              double_to_table("normal_results", "order4", &n4);
              augment_count("normal_results");
              string_to_table("normal_results", "name", "gnfs");
              double_to_table("normal_results", "order1", &n1);
              double_to_table("normal_results", "order2", &n2);
              double_to_table("normal_results", "order3", &n3);
              double_to_table("normal_results", "order4", &n4);
              augment_count("normal_results");
              string_to_table("normal_results", "name", "gnfa");
              double_to_table("normal_results", "order1", &n1);
              double_to_table("normal_results", "order2", &n2);
              double_to_table("normal_results", "order3", &n3);
              double_to_table("normal_results", "order4", &n4);
              augment_count("normal_results");
            }
            string_to_table("normal_results", "name", "gnfu");
            double_to_table("normal_results", "order1", &order[0]);
            double_to_table("normal_results", "order2", &order[1]);
            double_to_table("normal_results", "order3", &order[2]);
            double_to_table("normal_results", "order4", &order[3]);
            n1 = (double)mynres;
            double_to_table("normal_results", "value", &n1);
            augment_count("normal_results");
          }
        }
      }
      else
      {
        if(((strcmp(names[j], "dq1") == zero)|| (strcmp(names[j], "dq2") == zero)) && order[0] == zero) order[0]=one;
        string_to_table("normal_results", "name", names[j]);
        double_to_table("normal_results", "order1", &order[0]);
        double_to_table("normal_results", "order2", &order[1]);
        double_to_table("normal_results", "order3", &order[2]);
        double_to_table("normal_results", "order4", &order[3]);
        augment_count("normal_results");
        if(j == 12)
        {
          min_req_order = 1;
        }
        else
        {
          min_req_order = order[0]+order[1]+order[2];
          if (j >= 8) min_req_order += order[0]+order[1];
          if (j >= 6) min_req_order += 1;
        }
      }
      if (min_order < min_req_order) min_order = min_req_order;
    }
  }
  printf("The minimum required order is %d \n--------------------------------\n",min_order);
}
int select_ptc_idx()
{
  struct table* t;
  int pos;

  if ((pos = name_list_pos("normal_results", table_register->names)) > -1)
  {
    t = table_register->tables[pos];
    return t->curr;
  }
  else
    return pos;
}
int minimum_acceptable_order()
{
  return min_order;
}
void expand_line(struct char_p_array* l_buff)
  /* expands a beam line, applies rep. count and inversion */
{
  /* first get all bracket pairs with their level; keep max. level */
  int add=0, i=0, j=0, k=0, n=0, number=0, dummy=0, rep=-1, pos=0;
  int level = 0, l_max = 0, b_cnt = 0;
  char* p;
  struct int_array* lbpos = new_int_array(l_buff->curr);
  struct int_array* rbpos = new_int_array(l_buff->curr);
  struct int_array* b_level = new_int_array(l_buff->curr);

  for (i = 0; i < l_buff->curr; i++)
  {
    if (*l_buff->p[i] == '(')
    {
      lbpos->i[b_cnt] = i;
      b_level->i[b_cnt++] = level++;
      if (level > l_max) l_max = level;
    }
    else if (*l_buff->p[i] == ')')  level--;
  }
  l_max--;
  for (i = 0; i < b_cnt; i++)
    get_bracket_t_range(l_buff->p, '(', ')', lbpos->i[i],
                        l_buff->curr-1, &dummy, &rbpos->i[i]);
  lbpos->curr = rbpos->curr = b_level->curr = b_cnt;
  /* now loop over level from highest down to zero, expand '*' in each pair */
  for (level = l_max; level >=0; level--)
  {
    for (i = 0; i < b_cnt; i++)
    {
      if (b_level->i[i] == level && (pos = lbpos->i[i]) > 1)
      {
        if (*l_buff->p[pos-1] == '*')
        {
          sscanf(l_buff->p[pos-2], "%d", &rep);
          add = rep - 1;
          number = rbpos->i[i] - pos - 1; /* inside bracket */
          n = number * add; /* extra tokens */
          while (l_buff->curr + n >= l_buff->max)
            grow_char_p_array(l_buff);
          for (j = l_buff->curr; j > pos + number; j--) /* shift upwards */
            l_buff->p[j+n] = l_buff->p[j];
          l_buff->curr += n;
          for (k = 1; k <= add; k++)
          {
            for (j = pos+1; j <= pos+number; j++)
              l_buff->p[j+k*number] = tmpbuff(l_buff->p[j]);
          }
          for (j = 0; j < b_cnt; j++)  /* reset bracket pointers */
          {
            if (lbpos->i[j] > pos + number) lbpos->i[j] += n;
            if (rbpos->i[j] > pos + number) rbpos->i[j] += n;
          }
          l_buff->p[pos-1] = l_buff->p[pos-2] = blank;
        }
      }
    }
  }
  /* loop over buffer, expand simple element repetition */
  for (pos = 2; pos < l_buff->curr; pos++)
  {
    if (*l_buff->p[pos] == '*')
    {
      rep = -1;
      sscanf(l_buff->p[pos-1], "%d", &rep);
      if (rep < 0)
      {
        fatal_error("expand_line","Problem with reading number of copies");
      }
      n = add = rep - 1;
      while (l_buff->curr + n >= l_buff->max) grow_char_p_array(l_buff);
      for (j = l_buff->curr; j > pos + 1; j--) /* shift upwards */
        l_buff->p[j+n] = l_buff->p[j];
      l_buff->curr += n;
      for (k = 1; k <= add; k++)
      {
        j = pos+1;
        l_buff->p[j+k] = l_buff->p[j];
      }
      for (j = 0; j < b_cnt; j++)  /* reset bracket pointers */
      {
        if (lbpos->i[j] > pos + 1) lbpos->i[j] += n;
        if (rbpos->i[j] > pos + 1) rbpos->i[j] += n;
      }
      l_buff->p[pos-1] = l_buff->p[pos-2] = blank;
    }
  }
  /* get bracket pointers including new ones */
  while (b_level->max < l_buff->curr) grow_int_array(b_level);
  while (lbpos->max < l_buff->curr) grow_int_array(lbpos);
  while (rbpos->max < l_buff->curr) grow_int_array(rbpos);
  level = b_cnt = 0;
  for (i = 0; i < l_buff->curr; i++)
  {
    if (*l_buff->p[i] == '(')
    {
      lbpos->i[b_cnt] = i;
      b_level->i[b_cnt++] = level++;
    }
    else if (*l_buff->p[i] == ')')  level--;
  }
  for (i = 0; i < b_cnt; i++)
    get_bracket_t_range(l_buff->p, '(', ')', lbpos->i[i],
                        l_buff->curr-1, &dummy, &rbpos->i[i]);
  lbpos->curr = rbpos->curr = b_level->curr = b_cnt;
  /* now loop over level from highest down to zero, invert if '-' */
  for (level = l_max; level >= 0; level--)
  {
    for (i = 0; i < b_cnt; i++)
    {
      pos = lbpos->i[i];
      if (b_level->i[i] == level)
      {
        p = blank;
        for (j = pos - 1; j > 0; j--)
        {
          p = l_buff->p[j];
          if (*p != ' ')  break;
        }
        if (*p == '-')
        {
          number = rbpos->i[i] - pos - 1;
          n = number / 2;
          for (j = 0; j < n; j++)
          {
            p = l_buff->p[pos+j+1];
            l_buff->p[pos+j+1] = l_buff->p[pos+number-j];
            l_buff->p[pos+number-j] = p;
          }
        }
      }
    }
  }
  /* finally remove all non-alpha tokens */
  n = 0;
  for (i = 0; i < l_buff->curr; i++)
  {
    if (isalpha(*l_buff->p[i])) l_buff->p[n++] = l_buff->p[i];
  }
  l_buff->curr = n;
  lbpos = delete_int_array(lbpos);
  rbpos = delete_int_array(rbpos);
  b_level = delete_int_array(b_level);
}

void expand_curr_sequ(int flag)
  /* expands the current sequence, i.e. flattens it, inserts drifts etc. */
  /* The sequence length is updated - new feature HG 26.5.03 */
{
  char rout_name[] = "expand_curr_sequ";
  struct node* c_node;
  int j;
  current_sequ->end->at_value = current_sequ->end->position
    = sequence_length(current_sequ);
  if (current_sequ->ex_start != NULL)
  {
    current_sequ->ex_nodes = delete_node_list(current_sequ->ex_nodes);
    current_sequ->ex_start = delete_node_ring(current_sequ->ex_start);
    current_sequ->orbits = delete_vector_list(current_sequ->orbits);
  }
  if (current_sequ->ex_start == NULL)
  {
    use_count++;
    if (occ_list == NULL)
      occ_list = new_name_list("occ_list",10000);  /* for occurrence count */
    else occ_list->curr = 0;
    make_occ_list(current_sequ);
    all_node_pos(current_sequ);
    current_sequ->ex_nodes = new_node_list(2*current_sequ->nodes->curr);
    expand_sequence(current_sequ, flag);
    current_sequ->n_nodes =
      add_drifts(current_sequ->ex_start, current_sequ->ex_end);
    if (current_sequ->all_nodes != NULL) myfree(rout_name, current_sequ->all_nodes);
    current_sequ->all_nodes
      = (struct node**) mymalloc(rout_name, current_sequ->n_nodes * sizeof(struct node*));
    c_node = current_sequ->ex_start;
    for (j = 0; j < current_sequ->n_nodes; j++)
    {
      current_sequ->all_nodes[j] = c_node;
      c_node = c_node->next;
    }
  }
  set_node_bv(current_sequ); /* set bv factors for all nodes */
  if (current_range) set_range(current_range, current_sequ);
  else
  {
    current_sequ->range_start = current_sequ->ex_start;
    current_sequ->range_end = current_sequ->ex_end;
  }
}

void fill_beta0(struct command* beta0, struct node* node)
{
  /* makes uses of fact that beta0 and twiss_table have the same variables
     at the same place (+2)  up to energy inclusive */
  struct command_parameter_list* pl = beta0->par;
  struct name_list* nl = beta0->par_names;
  int i = -1, pos;
  if (twiss_table == NULL) return;
  for (pos = 0; pos < twiss_table->curr; pos++)
  {
    if (twiss_table->p_nodes[pos] == node)  break;
  }
  if (pos < twiss_table->curr)
  {
    do
    {
      i++;
      pl->parameters[i]->double_value = twiss_table->d_cols[i+3][pos];
/*      if (strstr(nl->names[i], "mu")) pl->parameters[i]->double_value *= twopi; frs 19.10.2006*/
    }
    while (strcmp(nl->names[i], "energy") != 0);
  }
}

void fill_constraint_list(int type /* 1 node, 2 global */,
                          struct command* cd, struct constraint_list* cl)
{
  struct command_parameter_list* pl = cd->par;
  struct name_list* nl = cd->par_names;
  struct constraint* l_cons;
  int j;
  for (j = 0; j < pl->curr; j++)
  {
    if (nl->inform[j] && pl->parameters[j]->type == 4)
    {
      l_cons = make_constraint(type, pl->parameters[j]);
      add_to_constraint_list(l_cons, cl);
    }
  }
}

void fill_orbit_table(struct table* t_out, struct table* t_in)
  /* fills a table with orbit values at monitor positions */
{
  int i, j, pos;
  t_out->curr = 0;
  for (i = 0; i < t_in->curr; i++)
  {
    if (strstr(t_in->s_cols[1][i], "monitor"))
    {
      for (j = 0; j < t_out->num_cols; j++)
      {
        if ((pos = name_list_pos(t_out->columns->names[j],
                                 t_in->columns)) > -1)
        {
          if (t_out->columns->inform[j] < 3)
            t_out->d_cols[j][t_out->curr] = t_in->d_cols[pos][i];
          else t_out->s_cols[j][t_out->curr]
                 = tmpbuff(t_in->s_cols[pos][i]);
        }
        else
        {
          if (t_out->columns->inform[j] < 3)
            t_out->d_cols[j][t_out->curr] = zero;
          else t_out->s_cols[j][t_out->curr] = tmpbuff(blank);
        }
      }
      t_out->curr++;
    }
  }
}

void fill_twiss_header(struct table* t)
  /* puts beam parameters etc. at start of twiss table */
{
  int i, pos, h_length = 39; /* change adding header lines ! */
  double dtmp;
  struct table* s;
  char tmp[16];

  if (t == NULL) return;
  /* ATTENTION: if you add header lines, augment h_length accordingly */
  if (t->header == NULL)  t->header = new_char_p_array(h_length);
  strcpy(tmp, t->org_sequ->name);
  sprintf(c_dum->c, v_format("@ SEQUENCE         %%%02ds \"%s\""),
          strlen(tmp),stoupper(tmp));
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  i = get_string("beam", "particle", tmp);
  sprintf(c_dum->c, v_format("@ PARTICLE         %%%02ds \"%s\""),
          i, stoupper(tmp));
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "mass");
  sprintf(c_dum->c, v_format("@ MASS             %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "charge");
  sprintf(c_dum->c, v_format("@ CHARGE           %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "energy");
  sprintf(c_dum->c, v_format("@ ENERGY           %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "pc");
  sprintf(c_dum->c, v_format("@ PC               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "gamma");
  sprintf(c_dum->c, v_format("@ GAMMA            %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "kbunch");
  sprintf(c_dum->c, v_format("@ KBUNCH           %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "bcurrent");
  sprintf(c_dum->c, v_format("@ BCURRENT         %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "sige");
  sprintf(c_dum->c, v_format("@ SIGE             %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "sigt");
  sprintf(c_dum->c, v_format("@ SIGT             %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "npart");
  sprintf(c_dum->c, v_format("@ NPART            %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "ex");
  sprintf(c_dum->c, v_format("@ EX               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "ey");
  sprintf(c_dum->c, v_format("@ EY               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "et");
  sprintf(c_dum->c, v_format("@ ET               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  if ((pos = name_list_pos("summ", table_register->names)) > -1)
  {
    s = table_register->tables[pos];
    pos = name_list_pos("length", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ LENGTH           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("alfa", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ ALFA             %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("orbit5", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ ORBIT5           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("gammatr", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ GAMMATR          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("q1", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ Q1               %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("q2", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ Q2               %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("dq1", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DQ1              %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("dq2", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DQ2              %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("dxmax", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DXMAX            %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("dymax", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DYMAX            %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("xcomax", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ XCOMAX           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("ycomax", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ YCOMAX           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("betxmax", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ BETXMAX          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("betymax", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ BETYMAX          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("xcorms", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ XCORMS           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("ycorms", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ YCORMS           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("dxrms", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DXRMS            %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("dyrms", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DYRMS            %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("deltap", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ DELTAP           %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("synch_1", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ SYNCH_1          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("synch_2", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ SYNCH_2          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("synch_3", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ SYNCH_3          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("synch_4", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ SYNCH_4          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    pos = name_list_pos("synch_5", s->columns);
    dtmp = s->d_cols[pos][0];
    sprintf(c_dum->c, v_format("@ SYNCH_5          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  }
}

void fill_twiss_header_ptc(struct table* t, double ptc_deltap)
  /* puts beam parameters etc. at start of twiss table */
{
  int i, h_length = 39; /* change adding header lines ! */
  double dtmp;
  /*  struct table* s; */
  char tmp[16];

  int returnStatus;
  int row;

  if (t == NULL) return;
  /* ATTENTION: if you add header lines, augment h_length accordingly */
  if (t->header == NULL)  t->header = new_char_p_array(h_length);
  strcpy(tmp, t->org_sequ->name);
  sprintf(c_dum->c, v_format("@ SEQUENCE         %%%02ds \"%s\""),
          strlen(tmp),stoupper(tmp));
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  i = get_string("beam", "particle", tmp);
  sprintf(c_dum->c, v_format("@ PARTICLE         %%%02ds \"%s\""),
          i, stoupper(tmp));
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "mass");
  sprintf(c_dum->c, v_format("@ MASS             %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "charge");
  sprintf(c_dum->c, v_format("@ CHARGE           %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "energy");
  sprintf(c_dum->c, v_format("@ ENERGY           %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "pc");
  sprintf(c_dum->c, v_format("@ PC               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "gamma");
  sprintf(c_dum->c, v_format("@ GAMMA            %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "kbunch");
  sprintf(c_dum->c, v_format("@ KBUNCH           %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "bcurrent");
  sprintf(c_dum->c, v_format("@ BCURRENT         %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "sige");
  sprintf(c_dum->c, v_format("@ SIGE             %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "sigt");
  sprintf(c_dum->c, v_format("@ SIGT             %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "npart");
  sprintf(c_dum->c, v_format("@ NPART            %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "ex");
  sprintf(c_dum->c, v_format("@ EX               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "ey");
  sprintf(c_dum->c, v_format("@ EY               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  dtmp = get_value("beam", "et");
  sprintf(c_dum->c, v_format("@ ET               %%le  %F"), dtmp);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
  sprintf(c_dum->c, v_format("@ DELTAP           %%le  %F"), ptc_deltap);
  t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

  /* one-turn information gets computed iff ptc_twiss_summary set to 1 in madx_ptc_twiss.f90 */
  if (get_option("ptc_twiss_summary") != zero){

    /* now retreive all pieces of information from the ptc_twiss*/

    row = 1; /* this particular table has only one row filled-in */

    /* momentum compaction factor, phase-slip factor and energy transition */
    returnStatus = double_from_table("ptc_twiss_summary","alpha_c", &row, &dtmp);
    /* returnStatus should always be equal to zero */
    sprintf(c_dum->c, v_format("@ ALPHA_C          %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","eta_c", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ ETA_C            %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","gamma_tr", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ GAMMA_TR         %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    /* tunes and chromaticities */
    returnStatus = double_from_table("ptc_twiss_summary","q1", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ Q1               %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","q2", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ Q2               %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","dq1", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ DQ1              %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","dq2", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ DQ2              %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    /* extremas of the beta-function */
    returnStatus = double_from_table("ptc_twiss_summary","beta_x_min", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ BETA_X_MIN       %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","beta_x_max", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ BETA_X_MAX       %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","beta_y_min", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ BETA_Y_MIN       %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);

    returnStatus = double_from_table("ptc_twiss_summary","beta_y_max", &row, &dtmp);
    sprintf(c_dum->c, v_format("@ BETA_Y_MAX       %%le  %F"), dtmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);


  }
}

struct command_list* find_command_list(char* name,
                                       struct command_list_list* sl)
{
  int pos;
  if ((pos = name_list_pos(name, sl->list)) < 0)
    return NULL;
  return sl->command_lists[pos];
}

void get_disp0(double* disp)
{
  copy_double(disp0, disp, 6);
}

int get_select_ex_ranges(struct sequence* sequ, struct command_list* select,
                         struct node_list* s_ranges)
  /* makes a list of nodes of an expanded sequence that pass the range
     selection */
{
  /*returns 0 if invalid sequence pointer
    1 if nodes in s_ranges (including 0) */
  struct name_list* nl;
  struct command* cd;
  struct command_parameter_list* pl;
  char* name;
  int full = 0, i, k, pos;
  struct node* c_node;
  struct node* nodes[2];
  if (sequ == NULL) return 0;
  s_ranges->curr = 0;
  s_ranges->list->curr = 0;
  for (i = 0; i < select->curr; i++)
  {
    cd = select->commands[i];
    nl = cd->par_names;
    pl = cd->par;
    pos = name_list_pos("full", nl);
    if ((pos = name_list_pos("full", nl)) > -1 && nl->inform[pos]
        && command_par_value("full", cd) != zero) full = 1;
    if (full == 0 && (pos = name_list_pos("range", nl)) > -1
        && nl->inform[pos])
    {
      name = pl->parameters[pos]->string;
      if ((k = get_ex_range(name, sequ, nodes)) == 0) return 0;
    }
    else
    {
      if ((nodes[0] = sequ->ex_start) == NULL ||
          (nodes[1] = sequ->ex_end) == NULL) return 0;
    }
    c_node = nodes[0];
    while (c_node != NULL)
    {
      if (full != 0 || pass_select(c_node->p_elem->name, cd) != 0)
        add_to_node_list(c_node, 0, s_ranges);
      if (c_node == nodes[1]) break;
      c_node = c_node->next;
    }
    if (full != 0) break;
  }
  return 1;
}

int get_select_ranges(struct sequence* sequ, struct command_list* select,
                      struct node_list* s_ranges)
  /* makes a list of nodes of a sequence that pass the range selection */
{
  struct name_list* nl;
  struct command_parameter_list* pl;
  char* name;
  char full_range[] = "#s/#e";
  int i, k, pos;
  struct node* c_node;
  struct node* nodes[2];
  for (i = 0; i < select->curr; i++)
  {
    nl = select->commands[i]->par_names;
    pl = select->commands[i]->par;
    pos = name_list_pos("range", nl);
    if (pos > -1 && nl->inform[pos])  /* parameter has been read */
      name = pl->parameters[pos]->string;
    else name = full_range;
    if ((k = get_range(name, sequ, nodes)) > 0)
    {
      c_node = nodes[0];
      while (c_node != NULL)
      {
        add_to_node_list(c_node, 0, s_ranges);
        if (c_node == nodes[1]) break;
        c_node = c_node->next;
      }
    }
  }
  return s_ranges->curr;
}

void get_select_t_ranges(struct command_list* select,
                         struct command_list* deselect, struct table* t)
  /* makes a list of table rows that pass the range selection and
     subsequent deselection */
{
  int rows[2];
  struct name_list* nl;
  struct command_parameter_list* pl;
  int i, pos;
  s_range->curr = 0; e_range->curr = 0;
  if (select != NULL)
  {
    for (i = 0; i < select->curr; i++)
    {
      nl = select->commands[i]->par_names;
      pl = select->commands[i]->par;
      pos = name_list_pos("range", nl);
      if (pos > -1 && nl->inform[pos]  /* parameter has been read */
          && get_table_range(pl->parameters[pos]->string, t, rows)
          && (rows[0] <= rows[1]))
      {
        if (s_range->max == s_range->curr) grow_int_array(s_range);
        if (e_range->max == e_range->curr) grow_int_array(e_range);
        s_range->i[s_range->curr++] = rows[0];
        e_range->i[e_range->curr++] = rows[1];
      }
      else
      {
        if (s_range->max == s_range->curr) grow_int_array(s_range);
        if (e_range->max == e_range->curr) grow_int_array(e_range);
        s_range->i[s_range->curr++] = 0;
        e_range->i[e_range->curr++] = t->curr - 1;
      }
    }
  }
  if (deselect != NULL)
  {
    for (i = 0; i < deselect->curr; i++)
    {
      nl = deselect->commands[i]->par_names;
      pl = deselect->commands[i]->par;
      pos = name_list_pos("range", nl);
      if (pos > -1 && nl->inform[pos]  /* parameter has been read */
          && get_table_range(pl->parameters[pos]->string, t, rows)
          && (rows[0] <= rows[1]))
      {
        if (sd_range->max == sd_range->curr) grow_int_array(sd_range);
        if (ed_range->max == ed_range->curr) grow_int_array(ed_range);
        sd_range->i[sd_range->curr++] = rows[0];
        ed_range->i[ed_range->curr++] = rows[1];
      }
    }
  }
}

int get_node_count(struct node* node)
  /* finds the count of a node in the current expanded sequence */
{
  int cnt = 0;
  current_node = current_sequ->ex_start;
  while (current_node != NULL)
  {
    if (current_node == node) return cnt;
    cnt++;
    if (current_node == current_sequ->ex_end) break;
    current_node = current_node->next;
  }
  return -1;
}

void get_node_vector(char* par, int* length, double* vector)
  /* returns vector for parameter par of current element */
{
  char lpar[NAME_L];
  mycpy(lpar, par);
  if (strcmp(lpar, "orbit0") == 0) copy_double(orbit0, vector, 6);
  else if (strcmp(lpar, "obs_orbit") == 0)
  {
    if (current_node->obs_orbit)
    {
      *length = current_node->obs_orbit->curr;
      copy_double(current_node->obs_orbit->a, vector, *length);
    }
    else *length = 0;
  }
  else if (strcmp(lpar, "orbit_ref") == 0)
  {
    if (current_node->orbit_ref)
    {
      *length = current_node->orbit_ref->curr;
      copy_double(current_node->orbit_ref->a, vector, *length);
    }
  }
  else *length = element_vector(current_node->p_elem, lpar, vector);
}

int get_ex_range(char* range, struct sequence* sequ, struct node** nodes)
  /* returns start and end node (nodes[0] and nodes[1])
     of a range in the full expanded sequence */
{
  int i, n, pos;
  char* c[2];
  char tmp[NAME_L];
  if (sequ == NULL) return 0;
  strcpy(c_dum->c, range); stolower(c_dum->c);
  c[0] = strtok(c_dum->c, "/");
  if ((c[1] = strtok(NULL,"/")) == NULL) /* only one element given */
    n = 1;
  else n = 2;
  for (i = 0; i < n; i++)
  {
    if (*c[i] == '#')
    {
      if (strncmp(c[i], "#s", 2) == 0) nodes[i] = sequ->ex_start;
      else if (strncmp(c[i], "#e", 2) == 0) nodes[i] = sequ->ex_end;
      else
      {
        warning("illegal expand range ignored:", range);
        return 0;
      }
    }
    else
    {
      strcpy(tmp, c[i]);
      if (square_to_colon(tmp) == 0)
      {
        warning("illegal expand range ignored:", range);
        return 0;
      }
      if ((pos =
           name_list_pos(tmp, sequ->ex_nodes->list)) > -1)
        nodes[i] = sequ->ex_nodes->nodes[pos];
      else
      {
        warning("illegal expand range ignored:", range);
        return 0;
      }
    }
  }
  if (n == 1) nodes[1] = nodes[0];
  return n;
}

int get_sub_range(char* range, struct sequence* sequ, struct node** nodes)
{
  /* returns start and end node (nodes[0] and nodes[1])
     of a range between range_start and range_end of an expanded sequence */
  int i, n;
  char* c[2];
  struct node* c_node;
  char tmp[NAME_L];
  if (sequ == NULL) return 0;
  strcpy(c_dum->c, range); stolower(c_dum->c);
  c[0] = strtok(c_dum->c, "/");
  if ((c[1] = strtok(NULL,"/")) == NULL) /* only one element given */
    n = 1;
  else n = 2;
  for (i = 0; i < n; i++)
  {
    if (*c[i] == '#')
    {
      if (strncmp(c[i], "#s", 2) == 0) nodes[i] = sequ->range_start;
      else if (strncmp(c[i], "#e", 2) == 0) nodes[i] = sequ->range_end;
      else
      {
        warning("illegal expand range ignored:", range);
        return 0;
      }
    }
    else
    {
      strcpy(tmp, c[i]);
      if (square_to_colon(tmp) == 0)
      {
        warning("illegal expand range ignored:", range);
        return 0;
      }
      c_node = sequ->range_start;
      while(c_node)
      {
        if (strcmp(c_node->name, tmp) == 0) break;
        if ((c_node = c_node->next) == sequ->range_end)
        {
          warning("illegal expand range ignored:", range);
          return 0;
        }
      }
      nodes[i] = c_node;
    }
  }
  if (n == 1) nodes[1] = nodes[0];
  return n;
}

double plot_option(char* name)
  /* returns the value of setplot parameters */
{
  double val = zero;
  int i;
  mycpy(c_dum->c, name);
  if (plot_options != NULL
      && (i = name_list_pos(c_dum->c, plot_options->par_names)) > -1)
    val = plot_options->par->parameters[i]->double_value;
  return val;
}

int get_range(char* range, struct sequence* sequ, struct node** nodes)
  /* returns start and end node (nodes[0] and nodes[1])
     of a range in the non-expanded sequence */
{
  int i, n, pos;
  char* c[2];
  char tmp[NAME_L];
  if (sequ == NULL) return 0;
  strcpy(c_dum->c, range); stolower(c_dum->c);
  c[0] = strtok(c_dum->c, "/");
  if ((c[1] = strtok(NULL,"/")) == NULL) /* only one element given */
    n = 1;
  else n = 2;
  for (i = 0; i < n; i++)
  {
    if (*c[i] == '#')
    {
      if (strncmp(c[i], "#s", 2) == 0) nodes[i] = sequ->start;
      else if (strncmp(c[i], "#e", 2) == 0) nodes[i] = sequ->end;
      else
      {
        warning("illegal range ignored:", range);
        return 0;
      }
    }
    else
    {
      strcpy(tmp, c[i]);
      if (square_to_colon(tmp) == 0)
      {
        warning("illegal range ignored:", range);
        return 0;
      }
      if ((pos =
           name_list_pos(tmp, sequ->nodes->list)) > -1)
        nodes[i] = sequ->nodes->nodes[pos];
      else
      {
        warning("illegal range ignored:", range);
        return 0;
      }
    }
  }
  if (n == 1) nodes[1] = nodes[0];
  return n;
}

int get_table_range(char* range, struct table* table, int* rows)
  /* returns start and end row (rows[0] and rows[1])
     of a range in a table; 0 if not found, 1 (1 row) or 2 ( > 1) */
{
  int i, n;
  char* c[2];
  char tmp[NAME_L], dumtex[3*NAME_L];;
  rows[0] = rows[1] = 0;
  mycpy(c_dum->c, range); stolower(c_dum->c); strcpy(dumtex, c_dum->c);
  c[0] = strtok(c_dum->c, "/");
  if ((c[1] = strtok(NULL,"/")) == NULL) /* only one element given */
    n = 1;
  else n = 2;
  for (i = 0; i < n; i++)
  {
    if (*c[i] == '#')
    {
      if (strncmp(c[i], "#s", 2) == 0) rows[i] = 0;
      else if (strncmp(c[i], "#e", 2) == 0) rows[i] = table->curr - 1;
      else
      {
        warning("illegal table range ignored:", dumtex);
        return 0;
      }
    }
    else
    {
      strcpy(tmp, c[i]);
      if (square_to_colon(tmp) == 0)
      {
        warning("illegal table range ignored:", dumtex);
        return 0;
      }
      if ((rows[i] = char_p_pos(tmp, table->node_nm)) < 0)
      {
        warning("illegal table range ignored:", dumtex);
        return 0;
      }
    }
  }
  if (n == 1) rows[1] = rows[0];
  return n;
}

void get_title(char* tlt, int* l)
  /* copies title from buffer into tl without trailing '\0' */
{
  *l = 0;
  if (title != NULL)
  {
    *l = strlen(title);
    strncpy(tlt, title, *l);
  }
}

int get_vector(char* name, char* par, double* vector)
  /* returns double "vector" for "par" of command or store "name";
     length is returned as function value (0 if not found) */
{
  mycpy(c_dum->c, name);
  mycpy(aux_buff->c, par);
  if (strcmp(c_dum->c, "threader") == 0)
    return command_par_vector(aux_buff->c, threader_par, vector);
  else return 0;
}

void get_version(char* tlt, int* l)
  /* returns version number */
{
  time_t tmp;
  struct tm* tm;
  int n = strlen(myversion);
  time(&tmp);
  tm = localtime(&tmp); /* split system time */
  strncpy(tlt, myversion, n);
  tlt += n;
  sprintf(tlt, "  %02d/%02d/%02d %02d.%02d.%02d\n",
          tm->tm_mday, tm->tm_mon+1, tm->tm_year%100,
          tm->tm_hour, tm->tm_min, tm->tm_sec);
  *l = n + 19;
}

void headvalue(char* table_name, char* par, double* value)
/* returns the value of header parameter par from table table_name if present,
   else 10^12 */
{
  int i, pos;
  char lpar[NAME_L], ltab[NAME_L];
  char* tp;
  struct table* tab;
  *value = ten_p_12;
  mycpy(ltab, table_name);
  stolower(ltab);
  if ((pos = name_list_pos(ltab, table_register->names)) > -1)
  {
    tab = table_register->tables[pos];
    mycpy(lpar, par);
    if (tab->header)
    {
      for (i = 0; i < tab->header->curr; i++)
      {
        strcpy(aux_buff->c, &tab->header->p[i][1]);
        if ((tp =strtok(aux_buff->c, " \"\n")) &&
            compare_no_case(tp, lpar) == 0)
        {
          if (strstr(strtok(NULL, " \"\n"), "%le") != NULL)
          {
            sscanf(strtok(NULL, " \"\n"), "%le", value);
            break;
          }
        }
      }
    }
  }
  return;
}

int int_in_array(int k, int n, int* array)
  /* returns 1 if k in first n elements of array, else 0 */
{
  int j;
  for (j = 0; j < n; j++)  if (k == array[j])  return 1;
  return 0;
}

void insert_elem(struct sequence* sequ, struct node* node)
  /* inserts an element in a sequence as function of its position */
{
  struct node* c_node = sequ->start->next;

  while (c_node != NULL)
  {
    if (node->position <= c_node->position || c_node == sequ->end) break;
    c_node = c_node->next;
  }
  link_in_front(node, c_node);
}

struct node* install_one(struct element* el, char* from_name, double at_value,
                         struct expression* at_expr, double position)
  /* adds an element to a sequence */
{
  struct node* node;
  int i, occ = 1;
  if (strcmp(el->base_type->name, "rfcavity") == 0 &&
      find_element(el->name, edit_sequ->cavities) == NULL)
    add_to_el_list(&el, 0, edit_sequ->cavities, 0);
  if ((i = name_list_pos(el->name, occ_list)) < 0)
    i = add_to_name_list(el->name, occ, occ_list);
  else occ = ++occ_list->inform[i];
  node = new_elem_node(el, occ);
  add_to_node_list(node, 0, edit_sequ->nodes);
  node->position = position;
  node->at_value = at_value;
  node->at_expr = at_expr;
  node->from_name = from_name;
  set_command_par_value("at", el->def, position);
  insert_elem(edit_sequ, node);
  return node;
}

int interp_node(int *nint)
{

  /* Creates interpolating nodes for the plotting routine */

  struct node *first_node, *clone;
  struct element* el;
  int j, number_nodes;
  double bvk, angle, e1, e2, h1, h2, fint, hgap;
  double zero = 0.0, minus_one = -1.0, length, step, numint;
  char *elem_name;
  int bend_flag = 0;

  numint = (double)*nint;
  number_nodes = *nint - 1;


  /* Set up length, angle and e2 of the first slice
     (first node in the original sequence) */

  first_node = current_node;
  el = first_node->p_elem;
  elem_name = el->base_type->name;
  rbend = (strcmp(elem_name, "rbend") == 0);
  bend_flag = (strcmp(elem_name, "sbend")*(rbend-1) == 0);

/*  bv = node_value("dipole_bv");*/
  bvk = node_value("other_bv");

  if (bend_flag)
  {
    angle = command_par_value("angle", el->def);
    e1 = command_par_value("e1", el->def);
    e2 = command_par_value("e2", el->def);
    h1 = command_par_value("h1", el->def);
    h2 = command_par_value("h2", el->def);
    fint = command_par_value("fint", el->def);
    fintx_plot = command_par_value("fintx", el->def);
    hgap = command_par_value("hgap", el->def);

    if (rbend)
    {
      e1 = e1 + bvk * angle / two;
      e2 = e2 + bvk * angle / two;
      strcpy(elem_name,"sbend");
      el->def->mad8_type = 3;
    }
    angle = angle/numint;
    store_node_value("angle",&angle);
    store_node_value("e1",&e1);
    store_node_value("e2",&zero);
    store_node_value("h1",&h1);
    store_node_value("h2",&zero);
    store_node_value("fint",&fint);
    store_node_value("fintx",&zero);
    store_node_value("hgap",&hgap);
  }
  length = first_node->length;
  step = length/numint;
  first_node->length = step;

  /* Set first_node in range_start of the sequence */

  current_sequ->range_start = first_node;

  /* clone the current node */

  clone = clone_node(first_node,0);
  if (bend_flag)
  {
    clone->p_elem = clone_element(first_node->p_elem);
    clone->p_elem->def = clone_command(first_node->p_elem->def);
  }

  /* Reset to first node */

  current_node = first_node;

  /* advance to next node */

  current_node = current_node->next;

  /* set last node in the range to the current node */

  current_sequ->range_end = current_node;


  /* insert nint - 1 nodes in between the two main nodes */

  for (j = 1; j <= number_nodes; j++)
  {
    link_in_front(clone,current_node);
    current_node = current_node->previous;
    current_node->previous->next = current_node;
    store_node_value("angle",&angle);
/*    store_node_value("dipole_bv",&bv); */
    store_node_value("other_bv",&bvk);
    if (bend_flag)
    {
      if (j == 1)
      {
        store_node_value("e2",&e2);
        store_node_value("h2",&h2);
        store_node_value("hgap",&hgap);
        if (fintx_plot < zero)
          store_node_value("fintx",&fint);
        else
          store_node_value("fintx",&fintx_plot);
        store_node_value("fint",&zero);
      }
      else
      {
        store_node_value("e2",&zero);
        store_node_value("h2",&zero);
        store_node_value("fint",&zero);
        store_node_value("fintx",&minus_one);
        store_node_value("hgap",&zero);
      }
      store_node_value("e1",&zero);
      store_node_value("h1",&zero);
    }
    clone = clone_node(first_node,0);
    if (bend_flag)
    {
      clone->p_elem = clone_element(first_node->p_elem);
      clone->p_elem->def = clone_command(first_node->p_elem->def);
    }
  }

  current_node = current_node->previous;

  return 0;
}

double line_nodes(struct char_p_array* flat)
  /* creates a linked node list from a flat element list of a line */
{
  int i, j, k;
  double pos = zero, val;
  struct element* el;
  for (j = 0; j < flat->curr; j++)
  {
    if ((el = find_element(flat->p[j], element_list)) == NULL)
      fatal_error("line contains unknown element:", flat->p[j]);
    if (strcmp(el->base_type->name, "rfcavity") == 0 &&
        find_element(el->name, current_sequ->cavities) == NULL)
      add_to_el_list(&el, 0, current_sequ->cavities, 0);
    val = el_par_value("l", el);
    pos += val / 2;
    k = 1;
    if ((i = name_list_pos(el->name, occ_list)) < 0)
      i = add_to_name_list(el->name, k, occ_list);
    else k = ++occ_list->inform[i];
    make_elem_node(el, k);
    current_node->at_value = current_node->position = pos;
    pos += val / 2;
  }
  return pos;
}

struct constraint* make_constraint(int type, struct command_parameter* par)
  /* makes + stores a constraint from command parameter */
{
  struct constraint* new = new_constraint(par->c_type);
  strcpy(new->name, par->name);
  switch(par->c_type)
  {
    case 1: /* minimum */
    case 3: /* both */
      if (par->min_expr == NULL) new->c_min = par->c_min;
      else
      {
        new->c_min = expression_value(par->min_expr, 2);
        new->ex_c_min = par->min_expr;
      }
      if (par->c_type == 1) break;
    case 2: /* maximum */
      if (par->max_expr == NULL) new->c_max = par->c_max;
      else
      {
        new->c_max = expression_value(par->max_expr, 2);
        new->ex_c_max = par->max_expr;
      }
      break;
    case 4: /* value */
      if (par->expr == NULL) new->value = par->double_value;
      else
      {
        new->value = expression_value(par->expr, 2);
        new->ex_value = par->expr;
      }
  }
  if (type == 1) new->weight = command_par_value(new->name, current_weight);
  else           new->weight = command_par_value(new->name, current_gweight);
  return new;
}

void make_sequ_from_line(char* name)
  /* converts a line into a sequence from actual line definition */
{
  char** tmp = NULL;
  int pos = name_list_pos(name, line_list->list);
  int spos;
  struct sequence* old_sequ = NULL;
  struct macro* line;
  int mpos = name_list_pos("marker", defined_commands->list);
  struct command* clone = clone_command(defined_commands->commands[mpos]);
  struct element* el;
  if (pos < 0) fatal_error("unknown line: ", name);
  line = line_list->macros[pos];
  line_buffer = new_char_p_array(1000);
  replace_lines(line, 0, tmp); /* replaces all referenced lines */
  expand_line(line_buffer); /* act on '-' and rep. count */
  current_sequ = new_sequence(name, 0); /* node positions = centre */
  if ((spos = name_list_pos(name, sequences->list)) >= 0)
    old_sequ = sequences->sequs[spos];
  add_to_sequ_list(current_sequ, sequences);
  if (old_sequ) old_sequ = delete_sequence(old_sequ);
  if (current_sequ->cavities != NULL)  current_sequ->cavities->curr = 0;
  else current_sequ->cavities = new_el_list(100);
  if (occ_list == NULL)
    occ_list = new_name_list("occ_list", 10000);  /* for occurrence count */
  else occ_list->curr = 0;
  sprintf(c_dum->c, "%s$start", current_sequ->name);
  el = make_element(c_dum->c, "marker", clone, 0);
  current_node = NULL;
  make_elem_node(el, 1);
  current_sequ->start = current_node;
  current_sequ->length = line_nodes(line_buffer);
  sprintf(c_dum->c, "%s$end", current_sequ->name);
  el = make_element(c_dum->c, "marker", clone, 0);
  make_elem_node(el, 1);
  current_node->at_value = current_node->position = current_sequ->length;
  current_sequ->end = current_node;
  current_sequ->start->previous = current_sequ->end;
  current_sequ->end->next = current_sequ->start;
  current_sequ->line = 1; /* remember origin of sequence */
  if(line_buffer) delete_char_p_array(line_buffer,1);
}

struct table* make_table(char* name, char* type, char** table_cols,
                         int* table_types, int rows)
{
  struct table* t;
  struct name_list *cols;
  struct command_list* scl;
  int i, n = 0;
  while (*table_cols[n] != ' ')
  {
/*     printf("make table %s col %d %s\n",name, n, table_cols[n]);*/
    n++;
  }
  cols = new_name_list("columns", n);
  for (i = 0; i < n; i++)
    add_to_name_list(table_cols[i], table_types[i], cols);
  if ((scl = find_command_list(name, table_select)) != NULL && scl->curr > 0)
    add_table_vars(cols, scl);
  t = new_table(name, type, rows, cols);
  t->org_cols = n;
  return t;
}

double mult_par(char* par, struct element* el)
  /* returns multipole parameter for par = "k0l" or "k0sl" etc. */
{
  char tmp[12];
  char* p;
  double val = zero, vect[FIELD_MAX];
  int k = 0, l, skew = 0;
  strcpy(tmp, par);
  if (*tmp == 'k' && (p = strchr(tmp, 'l')) != NULL)
  {
    *p = '\0';  /* suppress trailing l */
    if ((p = strchr(tmp, 's')) != NULL)
    {
      skew = 1; *p = '\0';
    }
    sscanf(&tmp[1], "%d", &k);
    if (skew) l = element_vector(el, "ksl", vect);
    else      l = element_vector(el, "knl", vect);
    if (k < l) val = vect[k];
  }
  return val;
}

int next_constraint(char* name, int* name_l, int* type, double* value,
                    double* c_min, double* c_max, double* weight)
  /* returns the parameters of the next constraint; 0 = none, else count */
{
  int i, ncp, nbl;
  struct constraint* c_c;
  int j,k;/* RDM fork */
  char s, takenextmacro, nomore; /* RDM fork */
  /* RDM fork */
  if (match_is_on==2) {
    i=match2_cons_curr[0];
    j=match2_cons_curr[1];
    k=match2_cons_curr[2];
    if(match2_cons_name[i][j]==NULL)
    {
      j++;
      if(j>=MAX_MATCH_CONS)
      {
        takenextmacro = 1;
      }
      else if (match2_cons_name[i][j]==NULL) /*if above is true this one can cause seg fault*/
      {
        takenextmacro = 1;
      }
      else
      {
        takenextmacro = 0;
      }

      if(takenextmacro)
      {
        i++;j=0;

        if(i>=MAX_MATCH_MACRO)
        {
          nomore = 1;
        }
        else if(match2_cons_name[i][j]==NULL)
        {
          nomore = 1;
        }
        else
        {/*i,j is the next constraint*/
          nomore = 0;
        }

        if( nomore == 0 ){
          name=match2_cons_name[i][j];
          *name_l=strlen(name);
          *type=2; /* to be changed according to s or <,=,>*/
          *value=match2_cons_value[i][j];
          s =match2_cons_sign[i][j];
          if (s == '>' && *value > 0) *value=0;
          else if (s == '<' && *value < 0) *value=0;
          c_min=value; /* unknown use */
          c_max=value; /* unknown use */
          *weight=1; /*hardcode no weight with this interface */
          k++;
          match2_cons_curr[0]=i;
          match2_cons_curr[1]=j;
          match2_cons_curr[2]=k;
          return k;
        } else {
          match2_cons_curr[0]=0;
          match2_cons_curr[1]=0;
          match2_cons_curr[2]=0;
          return 0;
        }
      }
    }
  }
  else  /* RDM old match */
  {
    if (current_node->cl == NULL) return 0;
    if (current_node->con_cnt == current_node->cl->curr)
    {
      current_node->con_cnt = 0; return 0;
    }
    c_c = current_node->cl->constraints[current_node->con_cnt];
    ncp = strlen(c_c->name) < *name_l ? strlen(c_c->name) : *name_l;
    nbl = *name_l - ncp;
    strncpy(name, c_c->name, ncp);
    for (i = 0; i < nbl; i++) name[ncp+i] = ' ';
    *type = c_c->type;
    if (c_c->ex_value == NULL) *value = c_c->value;
    else                       *value = expression_value(c_c->ex_value,2);
    if (c_c->ex_c_min == NULL) *c_min = c_c->c_min;
    else                       *c_min = expression_value(c_c->ex_c_min,2);
    if (c_c->ex_c_max == NULL) *c_max = c_c->c_max;
    else                       *c_max = expression_value(c_c->ex_c_max,2);
    *weight = c_c->weight;
    return ++current_node->con_cnt;
  }
  /* RDM fork */
  return 0;
}

int next_constr_namepos(char* name)
/* returns the Fortran (!) position of the named variable 
   in the opt_fun array of twiss */
{
  int pos = 0;
  switch (*name)
    {
    case 'a':
      if      (name[3] == 'x') pos = 4;
      else if (name[3] == 'y') pos = 7;
      break;
    case 'b':
      if      (name[3] == 'x') pos = 3;
      else if (name[3] == 'y') pos = 6;
      break;
    case 'd':
      if      (name[1] == 'x') pos = 15;
      else if (name[1] == 'y') pos = 17;
      else if (name[1] == 'p')
	{
          if      (name[2] == 'x') pos = 16;
          else if (name[2] == 'y') pos = 18;
	}
      else if (name[1] == 'm')
        {
          if      (name[3] == 'x') pos = 21;
          else if (name[3] == 'y') pos = 24;
        }
      else if (name[1] == 'd')
	{
	  if	  (name[2] == 'x') pos = 25;
	  else if (name[2] == 'y') pos = 27;
	  else if (name[2] == 'p')
	    {
	      if      (name[3] == 'x') pos = 26;
	      else if (name[3] == 'y') pos = 28;
	    }
	}
      break;
    case 'e':
      pos = 33;
      break;
    case 'm':
      if      (name[2] == 'x') pos = 5;
      else if (name[2] == 'y') pos = 8;
      break;
    case 'p':
      if      (name[1] == 'x') pos = 10;
      else if (name[1] == 'y') pos = 12;
      else if (name[1] == 't') pos = 14;
      else if (name[1] == 'h') pos = 14;
        {
	  if      (name[3] == 'x') pos = 20;
	  else if (name[3] == 'y') pos = 23;
	}
      break;
    case 'r':
      if      (name[1] == '1')
	{
          if      (name[2] == '1') pos = 29;
          else if (name[2] == '2') pos = 30;
	}
      else if      (name[1] == '2')
	{
          if      (name[2] == '1') pos = 31;
          else if (name[2] == '2') pos = 32;
	}
      break;
    case 't':
      pos = 13;
      break;
    case 'w':
      if      (name[1] == 'x') pos = 19;
      else if (name[1] == 'y') pos = 22;
      break;
    case 'x':
      pos = 9;
      break;
    case 'y':
      pos = 11;
    }
  return pos;
}

int constraint_name(char* name, int* name_l, int* index)
  /* returns the name of the constraint */
{
  int ncp, nbl;
  struct constraint* c_c;
  c_c = current_node->cl->constraints[*index];
  ncp = strlen(c_c->name) < *name_l ? strlen(c_c->name) : *name_l;
  nbl = *name_l - ncp;
  strncpy(name, c_c->name, ncp);
  return 1;
}


int next_global(char* name, int* name_l, int* type, double* value,
                double* c_min, double* c_max, double* weight)
  /* returns the parameters of the next global constraint;
     0 = none, else count */
{
  int i, ncp, nbl;
  struct constraint* c_c;
  if (current_sequ->cl == NULL) return 0;
  if (current_sequ->con_cnt == current_sequ->cl->curr)
  {
    current_sequ->con_cnt = 0; return 0;
  }
  c_c = current_sequ->cl->constraints[current_sequ->con_cnt];
  ncp = strlen(c_c->name) < *name_l ? strlen(c_c->name) : *name_l;
  nbl = *name_l - ncp;
  strncpy(name, c_c->name, ncp);
  for (i = 0; i < nbl; i++) name[ncp+i] = ' ';
  *type = c_c->type;
  if (c_c->ex_value == NULL) *value = c_c->value;
  else                       *value = expression_value(c_c->ex_value,2);
  if (c_c->ex_c_min == NULL) *c_min = c_c->c_min;
  else                       *c_min = expression_value(c_c->ex_c_min,2);
  if (c_c->ex_c_max == NULL) *c_max = c_c->c_max;
  else                       *c_max = expression_value(c_c->ex_c_max,2);
  *weight = c_c->weight;
  return ++current_sequ->con_cnt;
}

int next_start(double* x,double* px,double* y,double* py,double* t,
               double* deltae,double* fx,double* phix,double* fy,double* phiy,
               double* ft,double* phit)
  /* returns the parameters of the next particle to track;
     0 = none, else count */
{
  struct command* comm;
  if (start_cnt == stored_track_start->curr)
  {
    start_cnt = 0; return 0;
  }
  comm = stored_track_start->commands[start_cnt];
  *x = command_par_value("x", comm);
  *px = command_par_value("px", comm);
  *y = command_par_value("y", comm);
  *py = command_par_value("py", comm);
  *t = command_par_value("t", comm);
  *deltae = command_par_value("pt", comm);
  *fx = command_par_value("fx", comm);
  *phix = command_par_value("phix", comm);
  *fy = command_par_value("fy", comm);
  *phiy = command_par_value("phiy", comm);
  *ft = command_par_value("ft", comm);
  *phit = command_par_value("phit", comm);
  return ++start_cnt;
}

int next_vary(char* name, int* name_l,
              double* lower, double* upper, double* step,
              int* slope, double* opt)
  /* returns the next variable to be varied during match;
     0 = none, else count */
{
  int i, pos, ncp, nbl;
  double l_step;
  char* v_name;
  struct name_list* nl;
  struct command* comm;
  struct command_parameter_list* pl;
  if (vary_cnt == stored_match_var->curr)
  {
    vary_cnt = 0; return 0;
  }
  comm = stored_match_var->commands[vary_cnt];
  nl = comm->par_names;
  pl = comm->par;
  pos = name_list_pos("name", nl);
  v_name = pl->parameters[pos]->string;
  ncp = strlen(v_name) < *name_l ? strlen(v_name) : *name_l;
  nbl = *name_l - ncp;
  strncpy(name, v_name, ncp);
  for (i = 0; i < nbl; i++) name[ncp+i] = ' ';
  *lower = command_par_value("lower", comm);
  *upper = command_par_value("upper", comm);
  if ((l_step = command_par_value("step", comm)) < ten_m_12) l_step = ten_m_12;
  *step = l_step;
  *slope = command_par_value("slope", comm);
  *opt = command_par_value("opt", comm);
  return ++vary_cnt;
}

int vary_name(char* name, int* name_l, int* index)
  /* returns the variable name */
{
  int pos, ncp, nbl;
  char* v_name;
  struct name_list* nl;
  struct command* comm;
  struct command_parameter_list* pl;
  comm = stored_match_var->commands[*index];
  nl = comm->par_names;
  pl = comm->par;
  pos = name_list_pos("name", nl);
  v_name = pl->parameters[pos]->string;
  ncp = strlen(v_name) < *name_l ? strlen(v_name) : *name_l;
  nbl = *name_l - ncp;
  strncpy(name, v_name, ncp);
  return 1;
}


int node_al_errors(double* errors)
  /* returns the alignment errors of a node */
{
  if (current_node->p_al_err == NULL) return 0;
  else
  {
    copy_double(current_node->p_al_err->a, errors,
                current_node->p_al_err->curr);
    return current_node->p_al_err->curr;
  }
}

int node_fd_errors(double* errors)
  /* returns the field errors of a node */
{
  if (current_node->p_fd_err == NULL) return 0;
  else
  {
    copy_double(current_node->p_fd_err->a, errors,
                current_node->p_fd_err->curr);
    return current_node->p_fd_err->curr;
  }
}

void node_string(char* key, char* string, int* l)
  /* returns current node string value for "key" in Fortran format */
  /* l is max. allowed length in string */
{
  char tmp[2*NAME_L];
  char* p;
  int i, l_p, nbl, ncp = 0;
  mycpy(tmp, key);
  if ((p = command_par_string(tmp, current_node->p_elem->def)))
  {
    l_p = strlen(p);
    ncp = l_p < *l ? l_p : *l;
  }
  nbl = *l - ncp;
  for (i = 0; i < ncp; i++) string[i] = p[i];
  for (i = 0; i < nbl; i++) string[ncp+i] = ' ';
}

double spec_node_value(char* par, int* number)
  /* returns value for parameter par of specified node (start = 1 !!) */
{
  double value = zero;
  struct node* node = current_node;
  int n = *number + current_sequ->start_node - 1;
  if (0 <= n && n < current_sequ->n_nodes)
  {
    current_node = current_sequ->all_nodes[n];
    value = node_value(par);
    current_node = node;
  }
  return value;
}

void out_table(char* tname, struct table* t, char* filename)
  /* output of a table */
{
  int j;

  struct command_list* scl = find_command_list(tname, table_select);
  struct command_list* dscl = find_command_list(tname, table_deselect);
  while (t->num_cols > t->col_out->max)
    grow_int_array(t->col_out);
  while (t->curr > t->row_out->max)
    grow_int_array(t->row_out);
  t->row_out->curr = t->curr;
  if (par_present("full", NULL, scl))
    put_info("obsolete option 'full'"," ignored on 'select'");
  for (j = 0; j < t->curr; j++) t->row_out->i[j] = 1;
  for (j = 0; j < t->num_cols; j++) t->col_out->i[j] = j;
  t->col_out->curr = t->num_cols;
  if ((scl != NULL && scl->curr > 0) || (dscl != NULL && dscl->curr > 0))
  {
    set_selected_columns(t, scl);
    set_selected_rows(t, scl, dscl);
  }
  write_table(t, filename);
}

int par_present(char* par, struct command* cmd, struct command_list* c_list)
  /* returns 1 if in cmd or in c_list par is read, else returns 0 */
{
  struct name_list* nl;
  int i, pos;
  if (cmd != NULL)
  {
    nl = cmd->par_names;
    pos = name_list_pos(par, nl);
    if (pos > -1 && nl->inform[pos] > 0)  return 1;
  }
  if (c_list != NULL)
  {
    for (i = 0; i < c_list->curr; i++)
    {
      nl = c_list->commands[i]->par_names;
      pos = name_list_pos(par, nl);
      if (pos > -1 && nl->inform[pos] > 0)  return 1;
    }
  }
  return 0;
}

void pro_aperture(struct in_cmd* cmd)
{
  struct aper_node* limit_node;
  struct node *use_range[2];
  struct table* tw_cp;
  char *file, *range, tw_name[NAME_L], *table="aperture";
  int tw_cnt, rows;
  double interval;
  setbuf(stdout,(char *)NULL);

  embedded_twiss_cmd = cmd;

  /* check for valid sequence, beam and Twiss table */
  if (current_sequ != NULL && sequence_length(current_sequ) != zero)
  {
    if (attach_beam(current_sequ) == 0)
    {
      fatal_error("Aperture module - sequence without beam:",
                  current_sequ->name);
    }
  }
  else fatal_error("Aperture module - no active sequence:", current_sequ->name);

  if (current_sequ->tw_table == NULL)
  {
    warning("No TWISS table present","Aperture command ignored");
    return;
  }

  range = command_par_string("range", this_cmd->clone);
  if (get_ex_range(range, current_sequ, use_range) == 0)
  {
    warning("Illegal range.","Aperture command ignored");
    return;
  }
  current_node = use_range[0];

  /* navigate to starting point in Twiss table */
  tw_cp=current_sequ->tw_table;

  tw_cnt=1; /* table starts at 1 seen from char_from_table function */
  if (char_from_table(tw_cp->name, "name", &tw_cnt, tw_name) != 0)
  {
    warning("Erroneus Twiss table.","Aperture command ignored.");
    return;
  }
  aper_trim_ws(tw_name, NAME_L);
  while (strcmp(tw_name,current_node->name))
  {
    tw_cnt++;
    if (tw_cnt > tw_cp->curr)
    {
      warning("Could not find range start in Twiss table", "Aperture command ignored.");
      return;
    }
    char_from_table(tw_cp->name, "name", &tw_cnt, tw_name);
    aper_trim_ws(tw_name, NAME_L);
  }
  tw_cnt--; /* jumps back to "real" value */

  /* approximate # of needed rows in aperture table */
  interval = command_par_value("interval", this_cmd->clone);
  rows = current_sequ->n_nodes + 2 * (sequence_length(current_sequ)/interval);

  /* make empty aperture table */
  aperture_table=make_table(table, table, ap_table_cols, ap_table_types, rows);
  aperture_table->dynamic=1;
  add_to_table_list(aperture_table, table_register);

  /* calculate apertures and fill table */
  limit_node = aperture(table, use_range, tw_cp, &tw_cnt);

  if (limit_node->n1 != -1)
  {
    printf("\n\nAPERTURE LIMIT: %s, n1: %g, at: %g\n\n",
           limit_node->name,limit_node->n1,limit_node->s);
    file = command_par_string("file", this_cmd->clone);
    if (file != NULL)
    {
      aper_header(aperture_table, *limit_node);
      out_table(table, aperture_table, file);
    }
    if (strcmp(aptwfile,"dummy")) out_table(tw_cp->name, tw_cp, aptwfile);
  }
  else warning("Could not run aperture command.","Aperture command ignored");

  /* set pointer to updated Twiss table */
  current_sequ->tw_table=tw_cp;
}


void pro_emit(struct in_cmd* cmd)
  /* calls the emit module */
{
  char rout_name[] = "pro_emit";
  struct command* emit = cmd->clone;
  double e_deltap, e_tol, u0;
  int j, error, keep;
  double* tt;
  double emit_v[3], nemit_v[3], bmax[9], gmax[9], dismax[4], tunes[3],
    sig_v[4], pdamp[3];
  char tmp[16];

  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("sequence not active,", "EMIT ignored");
    return;
  }
  fprintf(prt_file, "enter EMIT module\n");
  if (attach_beam(current_sequ) == 0)
    fatal_error("EMIT - sequence without beam:", current_sequ->name);
  e_deltap = command_par_value("deltap", emit);
  e_tol = command_par_value("tol", emit);
  keep = get_option("twiss_print");
  j = 0;
  set_option("twiss_print", &j);
  zero_double(orbit0, 6);
  zero_double(disp0, 6);
  zero_double(oneturnmat, 36);
  tt = (double*) mycalloc("pro_emit", 216, sizeof(double));
  adjust_beam();
  probe_beam = clone_command(current_beam);
  tmrefe_(oneturnmat); /* one-turn linear transfer map */
  adjust_probe(e_deltap); /* sets correct gamma, beta, etc. */
  print_global(e_deltap);
  adjust_rfc(); /* sets freq in rf-cavities from probe */
  printf(v_format("guess: %I %F %F\n"),
         guess_flag, guess_orbit[0],guess_orbit[1]);
  if (guess_flag) copy_double(guess_orbit, orbit0, 6);
  getclor_(orbit0, oneturnmat, tt, &error); /* closed orbit */
  myfree(rout_name, tt);
  if (error == 0)
  {
    current_node = current_sequ->ex_start;
    emit_(&e_deltap, &e_tol, orbit0, disp0, oneturnmat, &u0, emit_v, nemit_v,
          bmax, gmax, dismax, tunes, sig_v, pdamp);
    if (e_deltap == zero)
    {
      store_comm_par_value("ex", emit_v[0], current_beam);
      store_comm_par_value("exn", nemit_v[0], current_beam);
      store_comm_par_value("ey", emit_v[1], current_beam);
      store_comm_par_value("eyn", nemit_v[1], current_beam);
      store_comm_par_value("et", emit_v[2], current_beam);
      store_comm_par_value("sigt", sig_v[2], current_beam);
      store_comm_par_value("sige", sig_v[3], current_beam);
      store_comm_par_value("u0", u0, current_beam);
      store_comm_par_value("qs", tunes[2], current_beam);
      store_comm_par_vector("pdamp", pdamp, current_beam);
    }
    else
    {
      sprintf(tmp, v_format("%F"), e_deltap);
      warning("EMIT: beam not updated, non-zero deltap: ", tmp);
    }
    print_rfc();
  }
  set_option("twiss_print", &keep);
}

void pro_ibs(struct in_cmd* cmd)
  /* control for IBS module */
{
  struct command* keep_beam = current_beam;
  struct name_list* nl = current_ibs->par_names;
  struct command_parameter_list* pl = current_ibs->par;
  char *filename = NULL, *table_name = NULL;
  int pos, w_file;

  if (twiss_table == NULL)
    warning("no TWISS table present","IBS command ignored");
  else
  {
    if ((current_beam
         = find_command(twiss_table->org_sequ->name, beam_list)) == NULL)
      current_beam = find_command("default_beam", beam_list);
    if (probe_beam != NULL) delete_command(probe_beam);
    probe_beam = clone_command(current_beam);
    pos = name_list_pos("file", nl);
    if (nl->inform[pos])
    {
      if ((filename = pl->parameters[pos]->string) == NULL)
      {
        if (pl->parameters[pos]->call_def != NULL)
          filename = pl->parameters[pos]->call_def->string;
      }
      if (filename == NULL) filename = permbuff("dummy");
      w_file = 1;
    }
    else w_file = 0;
    set_option("ibs_table", &w_file); /* fill only if output */
    if (w_file)
    {
      table_name = permbuff("ibs");
      ibs_table = make_table(table_name, "ibs", ibs_table_cols,
                             ibs_table_types, current_sequ->n_nodes);
      add_to_table_list(ibs_table, table_register);
    }
    adjust_probe(zero); /* sets correct gamma, beta, etc. */
    ibs_();
    if (w_file) out_table(table_name, ibs_table, filename);
    if (probe_beam) probe_beam = delete_command(probe_beam);
    current_beam = keep_beam;
  }
}

void pro_touschek(struct in_cmd* cmd)
  /* control for touschek module */
{
  struct command* keep_beam = current_beam;
  struct name_list* nl = current_touschek->par_names;
  struct command_parameter_list* pl = current_touschek->par;
  char *filename = NULL, *table_name = NULL;
  int pos, w_file;

  if (twiss_table == NULL)
    warning("no TWISS table present","touschek command ignored");
  else
  {

    if(get_option("centre"))
    {
      printf("Yes centre on \n");
    }
    else
    {
      printf("NO centre off \n");
    }
    if ((current_beam
         = find_command(twiss_table->org_sequ->name, beam_list)) == NULL)
      current_beam = find_command("default_beam", beam_list);
    if (probe_beam != NULL) delete_command(probe_beam);
    probe_beam = clone_command(current_beam);
    pos = name_list_pos("file", nl);
    if (nl->inform[pos])
    {
      if ((filename = pl->parameters[pos]->string) == NULL)
      {
        if (pl->parameters[pos]->call_def != NULL)
          filename = pl->parameters[pos]->call_def->string;
      }
      if (filename == NULL) filename = permbuff("dummy");
      w_file = 1;
    }
    else w_file = 0;
    set_option("touschek_table", &w_file); /* fill only if output */
    if (w_file)
    {
      table_name = permbuff("touschek");
      touschek_table = make_table(table_name, "touschek", touschek_table_cols,
                                  touschek_table_types, current_sequ->n_nodes);
      add_to_table_list(touschek_table, table_register);
    }
    adjust_probe(zero); /* sets correct gamma, beta, etc. */
    touschek_();
    if (w_file) out_table(table_name, touschek_table, filename);
    if (probe_beam) probe_beam = delete_command(probe_beam);
    current_beam = keep_beam;
  }
}

void pro_match(struct in_cmd* cmd)
  /* controls the matching module */
{
  /* OB 12.2.2002: changed the sequence of if statements so that MAD
     can go through the whole matching sequence */

  if (strcmp(cmd->tok_list->p[0], "match") == 0)
  {
    match_match(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "cell") == 0)
  {
    warning("CELL command no longer valid, ","use MATCH");
    return;
  }
  else if (match_is_on == 0)
  {
    warning("no MATCH command seen,","ignored");
    return;
  }
  else if (strcmp(cmd->tok_list->p[0], "endmatch") == 0)
  {
    match_end(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "migrad") == 0 ||
           strcmp(cmd->tok_list->p[0], "lmdif") == 0 ||
           strcmp(cmd->tok_list->p[0], "jacobian") == 0 ||
           strcmp(cmd->tok_list->p[0], "simplex") == 0)
  {
    match_action(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "constraint") == 0)
  {
    match_constraint(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "couple") == 0)
  {
    match_couple(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "fix") == 0)
  {
    match_fix(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "global") == 0)
  {
    match_global(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "level") == 0)
  {
    match_level(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "vary") == 0)
  {
    match_vary(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "weight") == 0)
  {
    match_weight(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "gweight") == 0)
  {
    match_gweight(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "rmatrix") == 0)
  {
    match_rmatrix(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "tmatrix") == 0)
  {
    match_tmatrix(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "global") == 0)
  {
    match_global(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "use_macro") == 0)
  {
    match2_macro(cmd);
  }
}

void pro_survey(struct in_cmd* cmd)
  /* calls survey module */
{
  struct name_list* nl = current_survey->par_names;
  struct command_parameter_list* pl = current_survey->par;
  char *filename = NULL, *table_name;
  int pos, w_file;
  int iarc = 1, keep;
  if (current_sequ == NULL)
  {
    warning("SURVEY, but no active sequence:", "ignored");
    return;
  }
  if (debuglevel > 1) fprintf(prt_file, "enter Survey module\n");
  keep = get_option("rbarc");
  set_option("rbarc", &iarc);
  pos = name_list_pos("file", nl);
  if (nl->inform[pos])
  {
    if ((filename = pl->parameters[pos]->string) == NULL)
    {
      if (pl->parameters[pos]->call_def != NULL)
        filename = pl->parameters[pos]->call_def->string;
    }
    if (filename == NULL) filename = permbuff("dummy");
    w_file = 1;
  }
  else w_file = 0;
  pos = name_list_pos("table", nl);
  if(nl->inform[pos]) /* table name specified - overrides save */
  {
    if ((table_name = pl->parameters[pos]->string) == NULL)
      table_name = pl->parameters[pos]->call_def->string;
  }
  else table_name = permbuff("survey");
  survey_table = make_table(table_name, "survey", survey_table_cols,
                            survey_table_types, current_sequ->n_nodes);
  add_to_table_list(survey_table, table_register);
  survey_();
  if (w_file) out_table(table_name, survey_table, filename);
  set_option("rbarc", &keep);
}

void pro_track(struct in_cmd* cmd)
  /* controls track module */
{
  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("TRACK, but no active sequence:", "ignored");
    return;
  }
  if (strcmp(cmd->tok_list->p[0], "track") == 0)
  {
    track_track(cmd);
  }
  if (strcmp(cmd->tok_list->p[0], "dynap") == 0)
  {
    track_dynap(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "endtrack") == 0)
  {
    track_end(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "observe") == 0)
  {
    track_observe(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "run") == 0)
  {
    track_run(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "ripple") == 0)
  {
    track_ripple(cmd);
  }
  else if (strcmp(cmd->tok_list->p[0], "start") == 0)
  {
    track_start(cmd->clone);
    cmd->clone_flag = 1;
  }
}

void pro_ptc_twiss()
  /* controls ptc_twiss module */
{
  struct command* keep_beam = current_beam;
  struct name_list* nl = current_twiss->par_names;
  struct command_parameter_list* pl = current_twiss->par;
  struct int_array* tarr;
  int ptc_twiss_summary = 0 ; /* set to 1 when a summary-table is filled-in */
  struct int_array* summary_tarr; /* to pass summary-table name to Fortran */
  struct node *nodes[2], *use_range[2];
  double ptc_deltap;
  char *filename = NULL, *table_name;
  char *summary_filename = NULL, *summary_table_name; /* for summary table */
  int j,l ,pos, w_file,beta_def;
  int w_file_summary; /* toggle to write the summary table into a file */
  /*
    start command decoding
  */
  use_range[0] = current_sequ->range_start;
  use_range[1] = current_sequ->range_end;
  if ((pos = name_list_pos("range", nl)) > -1 && nl->inform[pos])
  {
    if (get_sub_range(pl->parameters[pos]->string, current_sequ, nodes))
    {
      current_sequ->range_start = nodes[0];
      current_sequ->range_end = nodes[1];
    }
    else warning("illegal range ignored:", pl->parameters[pos]->string);
  }
  for (j = 0; j < current_sequ->n_nodes; j++)
  {
    if (current_sequ->all_nodes[j] == current_sequ->range_start) break;
  }

  if (attach_beam(current_sequ) == 0)
    fatal_error("PTC_TWISS - sequence without beam:", current_sequ->name);

  pos = name_list_pos("table", nl);
  if(nl->inform[pos]) /* table name specified - overrides save */
  {
    table_name = pl->parameters[pos]->string;
    if (table_name == NULL)
    {
      table_name = pl->parameters[pos]->call_def->string;
    }
  }
  else
  {
    /*strcpy(table_name,"ptc_twiss");*/
    table_name = "ptc_twiss";
  }

  /* --- */
  /* do the same as above for the table holding summary data after one-turn */
  pos = name_list_pos("summary_table",nl);
  if (nl->inform[pos]){ /* summary-table's name specified */
    summary_table_name = pl->parameters[pos]->string;
    if (summary_table_name == NULL){
      summary_table_name = pl->parameters[pos]->call_def->string;
    }
  }
  else {
    summary_table_name = "ptc_twiss_summary";
  }
  /* --- */

  pos = name_list_pos("file", nl);
  if (nl->inform[pos])
  {
    if ((filename = pl->parameters[pos]->string) == NULL)
    {
      if (pl->parameters[pos]->call_def != NULL)
        filename = pl->parameters[pos]->call_def->string;
    }
    if (filename == NULL) filename = permbuff("dummy");
    w_file = 1;
  }
  else w_file = 0;

  /* --- */
  /* do the same as above for the file to hold the summary-table */
  pos = name_list_pos("summary_file",nl);
  if (nl->inform[pos]){
    if ((summary_filename = pl->parameters[pos]->string) == NULL){
      if (pl->parameters[pos]->call_def != NULL)
        summary_filename = pl->parameters[pos]->call_def->string;
    }
    if (summary_filename == NULL) summary_filename = permbuff("dummy");
    w_file_summary = 1;
  }
  else w_file_summary = 0;

  /*
    end of command decoding
  */
  if ((beta_def = twiss_input(current_twiss)) < 0)
  {
    if (beta_def == -1) warning("unknown beta0,", "Twiss ignored");
    else if (beta_def == -2)
      warning("betx or bety missing,", "Twiss ignored");
    /*      set_variable("twiss_tol", &tol_keep); */
    return;
  }

  set_option("twiss_inval", &beta_def);
  adjust_beam();
  probe_beam = clone_command(current_beam);
  ptc_deltap = get_value(current_command->name,"deltap");
  adjust_probe(ptc_deltap); /* sets correct gamma, beta, etc. */
  adjust_rfc(); /* sets freq in rf-cavities from probe */
  l = strlen(table_name);
  tarr = new_int_array(l+1);
  conv_char(table_name, tarr);

  twiss_table = make_table(table_name, "twiss", twiss_table_cols,
                           twiss_table_types, current_sequ->n_nodes);

  twiss_table->dynamic = 1;
  add_to_table_list(twiss_table, table_register);
  current_sequ->tw_table = twiss_table;
  twiss_table->org_sequ = current_sequ;
  twiss_table->curr= 0;
  current_node = current_sequ->ex_start;
  /* w_ptc_twiss_(tarr->i); */

  /* --- */
  /* create additional table to hold summary data after one-turn */
  /* such as momentum compaction factor, tune and chromaticities */
  l = strlen(summary_table_name); /* reuse of l */
  summary_tarr = new_int_array(l+1);
  conv_char(summary_table_name, summary_tarr);

  ptc_twiss_summary_table = make_table(summary_table_name, "twiss summary",
                                       ptc_twiss_summary_table_cols, ptc_twiss_summary_table_types,
                                       5); /* only one  row would be enough  */
  ptc_twiss_summary_table->dynamic = 1; /* actually static for time-being */
  add_to_table_list( ptc_twiss_summary_table, table_register );
  /* --- */

  w_ptc_twiss(tarr->i,summary_tarr->i);

  /* upon completion of the Fortran ptc_twiss call ... */

  fill_twiss_header_ptc(twiss_table,ptc_deltap);
  if (w_file) out_table(table_name, twiss_table, filename);

  /* --- */
  if (w_file_summary) out_table(summary_table_name, ptc_twiss_summary_table,
                                summary_filename);
  /* --- */

  /* cleanup */
  current_beam = keep_beam;
  probe_beam = delete_command(probe_beam);
  current_sequ->range_start = use_range[0];
  current_sequ->range_end = use_range[1];
  delete_int_array(tarr);

  /* --- */
  delete_int_array(summary_tarr);
  /* --- */

  /* For the time-being, summary data are only available in case of a closed machine */
  ptc_twiss_summary = get_option("ptc_twiss_summary");
  if (ptc_twiss_summary) {
    print_table(ptc_twiss_summary_table);
  }
}

void pro_ptc_create_layout()
  /* controls ptc_create_layout module */
{
  int pos;
  struct command* keep_beam = current_beam;
  if (attach_beam(current_sequ) == 0)
    fatal_error("ptc_create_layout - sequence without beam:", current_sequ->name);
  adjust_beam();
  probe_beam = clone_command(current_beam);

  if ((pos = name_list_pos("errors_dipole", table_register->names)) <= -1)
  {
    errors_dipole = make_table("errors_dipole", "efield", efield_table_cols,
                               efield_table_types, 10000);
    add_to_table_list(errors_dipole, table_register);
  }
  else
  {
    reset_count("errors_dipole");
  }

  if ((pos = name_list_pos("errors_field", table_register->names)) <= -1)
  {
    errors_field = make_table("errors_field", "efield", efield_table_cols,
                              efield_table_types, 10000);
    add_to_table_list(errors_field, table_register);
  }
  else
  {
    reset_count("errors_field");
  }

  if ((pos = name_list_pos("errors_total", table_register->names)) <= -1)
  {
    errors_total = make_table("errors_total", "efield", efield_table_cols,
                              efield_table_types, 10000);
    add_to_table_list(errors_total, table_register);
  }
  else
  {
    reset_count("errors_total");
  }

  w_ptc_create_layout_();
  /* cleanup */
  current_beam = keep_beam;
  probe_beam = delete_command(probe_beam);
}

void pro_twiss()
  /* controls twiss module */
{
  struct command* keep_beam = current_beam;
  struct name_list* nl;
  struct command_parameter_list* pl;
  struct int_array* tarr;
  struct int_array* tarr_sector;
  struct node *nodes[2], *use_range[2];
  char *filename = NULL, *name, *table_name, *sector_name = NULL;
  char *sector_table_name = "dummy"; /* second string required by twiss() */
  /* will be set to a proper string in case twiss_sector option selected */
  double tol,tol_keep, q1_val_p = 0, q2_val_p = 0, q1_val, q2_val, dq1, dq2;
  int i, j, l, lp, k_orb = 0, u_orb = 0, pos, k_save = 1, k = 1, k_sect, 
      w_file, beta_def;
  int chrom_flg;
  int keep_info = get_option("info");
  i = keep_info * get_option("twiss_print");
  set_option("info", &i);

  if (current_twiss == 0x0)
  {
    seterrorflag(2,"pro_twiss","No twiss command seen yet");
    warning("pro_twiss","No twiss command seen yet!");
    return;
  }

  if (current_twiss->par_names == 0x0)
  {
    seterrorflag(3,"pro_twiss","Last twiss has NULL par_names pointer. Cannot proceed further.");
    warning("pro_twiss","Last twiss has NULL par_names pointer. Cannot proceed further.");
    return;
  }

  if (current_twiss->par == 0x0)
  {
    seterrorflag(4,"pro_twiss","Last twiss has NULL par pointer. Cannot proceed further.");
    warning("pro_twiss","Last twiss has NULL par pointer. Cannot proceed further.");
    return;
  }


  nl = current_twiss->par_names;
  pl = current_twiss->par;

  if (match_is_on)  k_save = 0;  /* match gets its own variable transfer -
				    this can be overridden with option "slow"
                                    on match command */

  /*
    start command decoding
  */
  pos = name_list_pos("sequence", nl);
  if(nl->inform[pos]) /* sequence specified */
  {
    name = pl->parameters[pos]->string;
    if ((lp = name_list_pos(name, sequences->list)) > -1)
      current_sequ = sequences->sequs[lp];
    else
    {
      warning("unknown sequence ignored:", name);
      return;
    }
  }
  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("sequence not active,", "Twiss ignored");
    return;
  }
  if(get_option("twiss_print")) fprintf(prt_file, "enter Twiss module\n");
  if (attach_beam(current_sequ) == 0)
    fatal_error("TWISS - sequence without beam:", current_sequ->name);
  pos = name_list_pos("table", nl);
  if(nl->inform[pos]) /* table name specified - overrides save */
  {
    if ((table_name = pl->parameters[pos]->string) == NULL)
      table_name = pl->parameters[pos]->call_def->string;
  }
  else if((pos = name_list_pos("save", nl)) > -1 &&
          nl->inform[pos]) /* save name specified */
  {
    k_save = 1;
    if ((table_name = pl->parameters[pos]->string) == NULL)
      table_name = pl->parameters[pos]->call_def->string;
  }
  else table_name = "twiss";
  if ((k_sect = get_value(current_command->name,"sectormap")) != 0)
  {
    set_option("twiss_sector", &k_sect);
    /* sector_table - start */
    pos = name_list_pos("sectortable",nl);
    if (nl->inform[pos]) {
      if ((sector_table_name = pl->parameters[pos]->string) == NULL)
        sector_table_name = pl->parameters[pos]->call_def->string;
    }
    else {
      sector_table_name = pl->parameters[pos]->call_def->string;
    }
    /* sector_table - end */
    pos = name_list_pos("sectorfile", nl);
    if(nl->inform[pos])
    {
      if ((sector_name = pl->parameters[pos]->string) == NULL)
        sector_name = pl->parameters[pos]->call_def->string;
    }
    else  sector_name = pl->parameters[pos]->call_def->string; /* filename for sector file */

  }
  use_range[0] = current_sequ->range_start;
  use_range[1] = current_sequ->range_end;
  if ((pos = name_list_pos("range", nl)) > -1 && nl->inform[pos])
  {
    if (get_sub_range(pl->parameters[pos]->string, current_sequ, nodes))
    {
      current_sequ->range_start = nodes[0];
      current_sequ->range_end = nodes[1];
    }
    else warning("illegal range ignored:", pl->parameters[pos]->string);
  }
  for (j = 0; j < current_sequ->n_nodes; j++)
  {
    if (current_sequ->all_nodes[j] == current_sequ->range_start) break;
  }
  if((pos = name_list_pos("useorbit", nl)) > -1 &&nl->inform[pos])
    /* orbit specified */
  {
    if (current_sequ->orbits == NULL)
      warning("orbit not found, ignored: ", pl->parameters[pos]->string);
    else
    {
      name = pl->parameters[pos]->string;
      if ((u_orb = name_list_pos(name, current_sequ->orbits->names)) < 0)
        warning("orbit not found, ignored: ", name);
      else set_option("useorbit", &k);
    }
  }
  pos = name_list_pos("centre", nl);
  if(nl->inform[pos])
    set_option("centre", &k);
  else
  {
    k = 0;
    set_option("centre", &k);
    k = 1;
  }
  pos = name_list_pos("keeporbit", nl);
  if(nl->inform[pos]) /* orbit specified */
  {
    name = pl->parameters[pos]->string;
    if (current_sequ->orbits == NULL)
      current_sequ->orbits = new_vector_list(10);
    else if (current_sequ->orbits->curr == current_sequ->orbits->max)
      grow_vector_list(current_sequ->orbits);
    if ((k_orb = name_list_pos(name, current_sequ->orbits->names)) < 0)
    {
      k_orb = add_to_name_list(permbuff(name), 0,
                               current_sequ->orbits->names);
      current_sequ->orbits->vectors[k_orb] = new_double_array(6);
    }
    set_option("keeporbit", &k);
  }
  pos = name_list_pos("file", nl);
  if (nl->inform[pos])
  {
    if ((filename = pl->parameters[pos]->string) == NULL)
    {
      if (pl->parameters[pos]->call_def != NULL)
        filename = pl->parameters[pos]->call_def->string;
    }
    if (filename == NULL) filename = permbuff("dummy");
    w_file = 1;

    strcpy(aptwfile,filename); /* IW 02.12.2004 */

  }
  else w_file = 0;
  tol_keep = get_variable("twiss_tol");
  pos = name_list_pos("tolerance", nl);
  if (nl->inform[pos])
  {
    tol = command_par_value("tolerance", current_twiss);
    set_variable("twiss_tol", &tol);
  }
  pos = name_list_pos("chrom", nl);
  chrom_flg = command_par_value("chrom", current_twiss);
  if((pos = name_list_pos("notable", nl)) > -1 &&nl->inform[pos]) k_save = 0;
  /*
    end of command decoding
  */

  zero_double(orbit0, 6);
  /*  zero_double(disp0, 6); */
  zero_double(oneturnmat, 36);
  if ((beta_def = twiss_input(current_twiss)) < 0)
  {
    if (beta_def == -1) warning("unknown beta0,", "Twiss ignored");
    else if (beta_def == -2)
      warning("betx or bety missing,", "Twiss ignored");
    set_variable("twiss_tol", &tol_keep);
    return;
  }
  set_option("twiss_inval", &beta_def);
  set_option("twiss_summ", &k);
  set_option("twiss_chrom", &chrom_flg);
  set_option("twiss_save", &k_save);
  set_twiss_deltas(current_twiss);
  adjust_beam();
  probe_beam = clone_command(current_beam);
  tmrefe_(oneturnmat); /* one-turn linear transfer map */
  summ_table = make_table("summ", "summ", summ_table_cols, summ_table_types,
                          twiss_deltas->curr+1);
  add_to_table_list(summ_table, table_register);
  l = strlen(table_name);
  tarr = new_int_array(l+1);
  conv_char(table_name, tarr);
  /* now create the sector table */
  l = strlen(sector_table_name);
  tarr_sector = new_int_array(l+1);
  conv_char(sector_table_name, tarr_sector);

  if (get_option("twiss_sector"))
  {
    reset_sector(current_sequ, 0);
    set_sector();

    twiss_sector_table = make_table(sector_table_name, "sectormap",
                                    twiss_sector_table_cols,
                                    twiss_sector_table_types,
                                    1); /* start with one row at first */
    twiss_sector_table->dynamic = 1; /* flag for access to current row */
    add_to_table_list(twiss_sector_table, table_register);
  }
  if (get_option("useorbit"))
    copy_double(current_sequ->orbits->vectors[u_orb]->a, orbit0, 6);
  else if (guess_flag)
  {
    for (i = 0; i < 6; i++)
    {
      if (guess_orbit[i] != zero) orbit0[i] = guess_orbit[i];
    }
  }

  for (i = 0; i < twiss_deltas->curr; i++)
  {
    if (chrom_flg) /* calculate chromaticity from tune difference - HG 6.2.09*/
    {
      twiss_table = make_table(table_name, "twiss", twiss_table_cols,
                               twiss_table_types, current_sequ->n_nodes);
      twiss_table->dynamic = 1; /* flag for table row access to current row */
      add_to_table_list(twiss_table, table_register);
      current_sequ->tw_table = twiss_table;
      twiss_table->org_sequ = current_sequ;
      adjust_probe(twiss_deltas->a[i]+DQ_DELTAP);
      adjust_rfc(); /* sets freq in rf-cavities from probe */
      current_node = current_sequ->ex_start;
      /* invoke twiss */
      twiss_(oneturnmat, disp0, tarr->i,tarr_sector->i);
      pos = name_list_pos("q1", summ_table->columns);
      q1_val_p = summ_table->d_cols[pos][i];
      pos = name_list_pos("q2", summ_table->columns);
      q2_val_p = summ_table->d_cols[pos][i];
    }
    if (k_save)
      {
    twiss_table = make_table(table_name, "twiss", twiss_table_cols,
                             twiss_table_types, current_sequ->n_nodes);
    twiss_table->dynamic = 1; /* flag for table row access to current row */
    add_to_table_list(twiss_table, table_register);
    current_sequ->tw_table = twiss_table;
    twiss_table->org_sequ = current_sequ;
      }
    adjust_probe(twiss_deltas->a[i]); /* sets correct gamma, beta, etc. */
    adjust_rfc(); /* sets freq in rf-cavities from probe */
    current_node = current_sequ->ex_start;
    /* invoke twiss */
    twiss_(oneturnmat, disp0, tarr->i,tarr_sector->i);
    augment_count_("summ ");
    if ((twiss_success = get_option("twiss_success")))
    {
      if (chrom_flg) /* calculate chromaticity from tune difference - HG 6.2.09*/
      {
        pos = name_list_pos("q1", summ_table->columns);
        q1_val = summ_table->d_cols[pos][i];
        pos = name_list_pos("q2", summ_table->columns);
        q2_val = summ_table->d_cols[pos][i];
        dq1 = (q1_val_p - q1_val) / DQ_DELTAP;
        dq2 = (q2_val_p - q2_val) / DQ_DELTAP;
        pos = name_list_pos("dq1", summ_table->columns);
        summ_table->d_cols[pos][i] = dq1;
        pos = name_list_pos("dq2", summ_table->columns);
        summ_table->d_cols[pos][i] = dq2;
      }
      if (get_option("keeporbit"))  copy_double(orbit0,
                                                current_sequ->orbits->vectors[k_orb]->a, 6);
      if (k_save) fill_twiss_header(twiss_table);
      if (i == 0) exec_savebeta(); /* fill beta0 at first delta_p only */
      if (k_save && w_file) out_table(table_name, twiss_table, filename);
    }
    else
    {
      seterrorflag(1,"pro_twiss","TWISS failed");
      warning("Twiss failed: ", "MAD-X continues");
    }
  }
  if (get_option("twiss_sector")){
    out_table( sector_table_name, twiss_sector_table, sector_name );
  }

  tarr = delete_int_array(tarr);
  tarr_sector = delete_int_array(tarr_sector);

  if (twiss_success && get_option("twiss_print")) print_table(summ_table);

  /* cleanup */
  current_beam = keep_beam;
  probe_beam = delete_command(probe_beam);
  k = 0;
  set_option("couple", &k);
  set_option("chrom", &k);
  set_option("rmatrix", &k);
  set_option("twiss_sector", &k);
  set_option("keeporbit", &k);
  set_option("useorbit", &k);
  set_option("info", &keep_info);
  set_variable("twiss_tol", &tol_keep);
  current_sequ->range_start = use_range[0];
  current_sequ->range_end = use_range[1];
}

void pro_embedded_twiss(struct command* current_global_twiss)
  /* controls twiss embedded module */
{
  struct command* keep_beam = current_beam;
  struct command* keep_twiss;
  struct name_list* nl = current_twiss->par_names;
  struct command_parameter_list* pl = current_twiss->par;
  struct int_array* tarr;
  struct int_array* dummy_arr; /* for the new signature of the twiss() Fortran function*/
  struct table* twiss_tb;
  struct table* keep_table = NULL;
  char *filename = NULL, *name, *table_name, *sector_name;
  char *table_embedded_name;
  double tol,tol_keep;
  double betx,bety,alfx,mux,alfy,muy,x,px,y,py,t,pt,dx,dpx,dy,dpy,wx,
    phix,dmux,wy,phiy,dmuy,ddx,ddpx,ddy,ddpy,
    r11,r12,r21,r22,s;
  int i, jt=0, l, lp, k_orb = 0, u_orb = 0, pos, k = 1;
  int ks, w_file, beta_def, err = 0, inval = 1,chrom_flg;
  int keep_info = get_option("info");

  /* Set embedded_flag */

  embedded_flag = 1;

  i = keep_info * get_option("twiss_print");
  set_option("info", &i);
  /*
    start command decoding
  */
  pos = name_list_pos("sequence", nl);
  if(nl->inform[pos]) /* sequence specified */
  {
    name = pl->parameters[pos]->string;
    if ((lp = name_list_pos(name, sequences->list)) > -1)
      current_sequ = sequences->sequs[lp];
    else
    {
      warning("unknown sequence ignored:", name);
      return;
    }
  }
  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("sequence not active,", "Twiss ignored");
    return;
  }
  if(get_option("twiss_print")) fprintf(prt_file, "enter Twiss module\n");
  if (attach_beam(current_sequ) == 0)
    fatal_error("TWISS - sequence without beam:", current_sequ->name);

  table_name = current_sequ->tw_table->name;
  table_embedded_name = "embedded_twiss_table";

  if ((ks = get_value(current_command->name,"sectormap")) != 0)
  {
    set_option("twiss_sector", &k);
    pos = name_list_pos("sectorfile", nl);
    if(nl->inform[pos])
    {
      if ((sector_name = pl->parameters[pos]->string) == NULL)
        sector_name = pl->parameters[pos]->call_def->string;
    }
    else  sector_name = pl->parameters[pos]->call_def->string;
    if ((sec_file = fopen(sector_name, "w")) == NULL)
      fatal_error("cannot open output file:", sector_name);
  }

  /* Find index to the twiss table */

  if((pos = name_list_pos(table_name, table_register->names)) > -1)
  {
    twiss_tb = table_register->tables[pos];
    if (twiss_tb->origin ==1) return; /* table is read, has no node pointers */
    for (jt = 0; jt < twiss_tb->curr; jt++)
    {
      if (twiss_tb->p_nodes[jt] == current_sequ->range_start) break;
    }
  }
  if((pos = name_list_pos("useorbit", nl)) > -1 &&nl->inform[pos])
    /* orbit specified */
  {
    if (current_sequ->orbits == NULL)
      warning("orbit not found, ignored: ", pl->parameters[pos]->string);
    else
    {
      name = pl->parameters[pos]->string;
      if ((u_orb = name_list_pos(name, current_sequ->orbits->names)) < 0)
        warning("orbit not found, ignored: ", name);
      else set_option("useorbit", &k);
    }
  }
  pos = name_list_pos("keeporbit", nl);
  if(nl->inform[pos]) /* orbit specified */
  {
    name = pl->parameters[pos]->string;
    if (current_sequ->orbits == NULL)
      current_sequ->orbits = new_vector_list(10);
    else if (current_sequ->orbits->curr == current_sequ->orbits->max)
      grow_vector_list(current_sequ->orbits);
    if ((k_orb = name_list_pos(name, current_sequ->orbits->names)) < 0)
    {
      k_orb = add_to_name_list(permbuff(name), 0,
                               current_sequ->orbits->names);
      current_sequ->orbits->vectors[k_orb] = new_double_array(6);
    }
    set_option("keeporbit", &k);
  }
  pos = name_list_pos("file", nl);
  if (nl->inform[pos])
  {
    if ((filename = pl->parameters[pos]->string) == NULL)
    {
      if (pl->parameters[pos]->call_def != NULL)
        filename = pl->parameters[pos]->call_def->string;
    }
    if (filename == NULL) filename = permbuff("dummy");
    w_file = 1;
  }
  else w_file = 0;
  tol_keep = get_variable("twiss_tol");
  pos = name_list_pos("tolerance", nl);
  if (nl->inform[pos])
  {
    tol = command_par_value("tolerance", current_twiss);
    set_variable("twiss_tol", &tol);
  }
  chrom_flg = command_par_value("chrom", current_twiss);

  /*
    end of command decoding
  */

  zero_double(orbit0, 6);
  /*  zero_double(disp0, 6); */
  zero_double(oneturnmat, 36);

  /* Initialise Twiss parameters */

  keep_twiss = current_twiss;

  if ((beta_def = twiss_input(current_twiss)) < 0)
  {
    if (beta_def == -1) warning("unknown beta0,", "Twiss ignored");
    else if (beta_def == -2)
      warning("betx or bety missing,", "Twiss ignored");
    set_variable("twiss_tol", &tol_keep);
    return;
  }
  set_option("twiss_inval", &beta_def);
  set_option("twiss_summ", &k);
  set_option("twiss_chrom", &chrom_flg);
  set_option("twiss_save", &k);

  /* Read Twiss parameters */

  current_twiss = current_global_twiss;

  /*jt is the row number of previous element*/

  if (jt <= 0) err = 1;
  if (err == 0)
  {
    err = double_from_table(table_name, "betx", &jt, &betx);
    err = double_from_table(table_name, "bety", &jt, &bety);
    err = double_from_table(table_name, "alfx", &jt, &alfx);
    err = double_from_table(table_name, "mux", &jt, &mux);
    /* mux = mux*twopi; frs 19.10.2006 */
    err = double_from_table(table_name, "alfy", &jt, &alfy);
    err = double_from_table(table_name, "muy", &jt, &muy);
    /* muy = muy*twopi; frs 19.10.2006 */
    err = double_from_table(table_name, "x", &jt, &x);
    err = double_from_table(table_name, "px", &jt, &px);
    err = double_from_table(table_name, "y", &jt, &y);
    err = double_from_table(table_name, "py", &jt, &py);
    err = double_from_table(table_name, "t", &jt, &t);
    err = double_from_table(table_name, "pt", &jt, &pt);
    err = double_from_table(table_name, "dx", &jt, &dx);
    err = double_from_table(table_name, "dpx", &jt, &dpx);
    err = double_from_table(table_name, "dy", &jt, &dy);
    err = double_from_table(table_name, "dpy", &jt, &dpy);
    err = double_from_table(table_name, "wx", &jt, &wx);
    err = double_from_table(table_name, "phix", &jt, &phix);
    err = double_from_table(table_name, "dmux", &jt, &dmux);
    err = double_from_table(table_name, "wy", &jt, &wy);
    err = double_from_table(table_name, "phiy", &jt, &phiy);
    err = double_from_table(table_name, "dmuy", &jt, &dmuy);
    err = double_from_table(table_name, "ddx", &jt, &ddx);
    err = double_from_table(table_name, "ddpx", &jt, &ddpx);
    err = double_from_table(table_name, "ddy", &jt, &ddy);
    err = double_from_table(table_name, "ddpy", &jt, &ddpy);
    err = double_from_table(table_name, "r11",&jt, &r11);
    err = double_from_table(table_name, "r12",&jt, &r12);
    err = double_from_table(table_name, "r21",&jt, &r21);
    err = double_from_table(table_name, "r22",&jt, &r22);
    err = double_from_table(table_name, "s",&jt, &s);

    /* Store these Twiss parameters as initial values */

    current_twiss = keep_twiss;
    set_value("twiss", "betx" , &betx);
    nl->inform[name_list_pos("betx",nl)] = 1;
    set_value("twiss", "bety" , &bety);
    nl->inform[name_list_pos("bety",nl)] = 1;
    set_value("twiss", "alfx" , &alfx);
    nl->inform[name_list_pos("alfx",nl)] = 1;
    set_value("twiss", "mux", &mux);
    nl->inform[name_list_pos("mux",nl)] = 1;
    set_value("twiss", "alfy", &alfy);
    nl->inform[name_list_pos("alfy",nl)] = 1;
    set_value("twiss", "muy", &muy);
    nl->inform[name_list_pos("muy",nl)] = 1;
    set_value("twiss", "x", &x);
    nl->inform[name_list_pos("x",nl)] = 1;
    set_value("twiss", "px", &px);
    nl->inform[name_list_pos("px",nl)] = 1;
    set_value("twiss", "y", &y);
    nl->inform[name_list_pos("y",nl)] = 1;
    set_value("twiss", "py", &py);
    nl->inform[name_list_pos("py",nl)] = 1;
    set_value("twiss", "t", &t);
    nl->inform[name_list_pos("t",nl)] = 1;
    set_value("twiss", "pt", &pt);
    nl->inform[name_list_pos("pt",nl)] = 1;
    set_value("twiss", "dx", &dx);
    nl->inform[name_list_pos("dx",nl)] = 1;
    set_value("twiss", "dpx", &dpx);
    nl->inform[name_list_pos("dpx",nl)] = 1;
    set_value("twiss", "dy", &dy);
    nl->inform[name_list_pos("dy",nl)] = 1;
    set_value("twiss", "dpy", &dpy);
    nl->inform[name_list_pos("dpy",nl)] = 1;
    set_value("twiss", "wx", &wx);
    nl->inform[name_list_pos("wx",nl)] = 1;
    set_value("twiss", "phix", &phix);
    nl->inform[name_list_pos("phix",nl)] = 1;
    set_value("twiss", "dmux", &dmux);
    nl->inform[name_list_pos("dmux",nl)] = 1;
    set_value("twiss", "wy", &wy);
    nl->inform[name_list_pos("wy",nl)] = 1;
    set_value("twiss", "phiy", &phiy);
    nl->inform[name_list_pos("phiy",nl)] = 1;
    set_value("twiss", "dmuy", &dmuy);
    nl->inform[name_list_pos("dmuy",nl)] = 1;
    set_value("twiss", "ddx", &ddx);
    nl->inform[name_list_pos("ddx",nl)] = 1;
    set_value("twiss", "ddpx", &ddpx);
    nl->inform[name_list_pos("ddpx",nl)] = 1;
    set_value("twiss", "ddy", &ddy);
    nl->inform[name_list_pos("ddy",nl)] = 1;
    set_value("twiss", "ddpy", &ddpy);
    nl->inform[name_list_pos("ddpy",nl)] = 1;
    set_value("twiss", "r11", &r11);
    nl->inform[name_list_pos("r11",nl)] = 1;
    set_value("twiss", "r12", &r12);
    nl->inform[name_list_pos("r12",nl)] = 1;
    set_value("twiss", "r21", &r21);
    nl->inform[name_list_pos("r21",nl)] = 1;
    set_value("twiss", "r22", &r22);
    nl->inform[name_list_pos("r22",nl)] = 1;

    adjust_beam();
    probe_beam = clone_command(current_beam);
    tmrefe_(oneturnmat); /* one-turn linear transfer map */
    summ_table = make_table("summ", "summ", summ_table_cols, summ_table_types,
                            twiss_deltas->curr+1);
    add_to_table_list(summ_table, table_register);
    l = strlen(table_embedded_name);
    tarr = new_int_array(l+1);
    conv_char(table_embedded_name, tarr);
    dummy_arr = new_int_array(5+1);
    conv_char("dummy",dummy_arr);
    if (get_option("twiss_sector"))
    {
      reset_sector(current_sequ, 0);
      set_sector();
    }

    if (get_option("useorbit"))
      copy_double(current_sequ->orbits->vectors[u_orb]->a, orbit0, 6);
    else if (guess_flag)
    {
      for (i = 0; i < 6; i++)
      {
        if (guess_orbit[i] != zero) orbit0[i] = guess_orbit[i];
      }
    }

    if(twiss_deltas->curr <= 0)
      fatal_error("PRO_TWISS_EMBEDDED "," - No twiss deltas");

    for (i = 0; i < twiss_deltas->curr; i++)
    {
      twiss_table = make_table(table_embedded_name, "twiss", twiss_table_cols,
                               twiss_table_types, current_sequ->n_nodes);

      twiss_table->dynamic = 1; /* flag for table row access to current row */

      add_to_table_list(twiss_table, table_register);

      keep_table = current_sequ->tw_table;
      current_sequ->tw_table = twiss_table;

      twiss_table->org_sequ = current_sequ;
      adjust_probe(twiss_deltas->a[i]); /* sets correct gamma, beta, etc. */

      adjust_rfc(); /* sets freq in rf-cavities from probe */
      current_node = current_sequ->range_start;
      set_option("twiss_inval", &inval);

      twiss_(oneturnmat, disp0, tarr->i, dummy_arr->i); /* different call */

      if ((twiss_success = get_option("twiss_success")))
      {
        if (get_option("keeporbit"))  copy_double(orbit0,
                                                  current_sequ->orbits->vectors[k_orb]->a, 6);
        fill_twiss_header(twiss_table);
        if (i == 0) exec_savebeta(); /* fill beta0 at first delta_p only */
        if (w_file) out_table(table_embedded_name, twiss_table, filename);
      }
      else warning("Twiss failed: ", "MAD-X continues");
    }

    if (sec_file)
    {
      fclose(sec_file); sec_file = NULL;
    }
    tarr = delete_int_array(tarr);
    dummy_arr = delete_int_array(dummy_arr);
    if (twiss_success && get_option("twiss_print")) print_table(summ_table);
  }
  else warning("Embedded Twiss failed: ", "MAD-X continues");
  /* cleanup */
  current_beam = keep_beam;
  probe_beam = delete_command(probe_beam);
  set_option("twiss_print", &k);
  k = 0;
  set_option("couple", &k);
  set_option("chrom", &k);
  set_option("rmatrix", &k);
  /* set_option("centre", &k); */
  set_option("twiss_sector", &k);
  set_option("keeporbit", &k);
  set_option("useorbit", &k);
  set_option("info", &keep_info);
  set_variable("twiss_tol", &tol_keep);
  current_sequ->tw_table = keep_table;

  /* Reset embedded_flag */

  embedded_flag = 0;
}

int embedded_twiss()
  /* controls twiss module to create a twiss table for interpolated nodes
     between two elements */

{
  struct name_list* tnl; /* OB 31.1.2002: local name list for TWISS input definition */
  struct in_cmd* cmd;
  struct command* current_global_twiss;
  struct command_parameter* cp;
  struct name_list* nl;
  struct command_parameter_list* pl;
  char* embedded_twiss_beta[2];
  int j, pos, tpos;
  int izero = 0;

  cmd = embedded_twiss_cmd;
  nl = cmd->clone->par_names;
  pl = cmd->clone->par;
  keep_tw_print = get_option("twiss_print");
  set_option("twiss_print", &izero);

  /* START defining a TWISS input command for default sequence */

  local_twiss[0] = new_in_cmd(10);
  local_twiss[0]->type = 0;
  local_twiss[0]->clone = local_twiss[0]->cmd_def
    = clone_command(find_command("twiss", defined_commands));
  tnl = local_twiss[0]->cmd_def->par_names;
  tpos = name_list_pos("sequence", tnl);
  local_twiss[0]->cmd_def->par->parameters[tpos]->string = current_sequ->name;
  local_twiss[0]->cmd_def->par_names->inform[tpos] = 1;

  /* END defining a TWISS input command for default sequence */

  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("Command called without active sequence,", "ignored");
    return 1;
  }
  /* END CHK-SEQ; OB 1.2.2002 */

  for (j = 0; j < local_twiss[0]->cmd_def->par->curr; j++)
  {
    tnl = local_twiss[0]->cmd_def->par_names;
    tpos = name_list_pos("sequence", tnl);
    if (j != tpos) local_twiss[0]->cmd_def->par_names->inform[j] = 0;
  }

  /* START CHK-BETA-INPUT; OB 1.2.2002 */
  /* START CHK-BETA0; OB 23.1.2002 */
  pos = name_list_pos("beta0", nl);
  if (pos > -1 && nl->inform[pos])  /* parameter has been read */
  {
    /* beta0 specified */
    cp = cmd->clone->par->parameters[pos];
    embedded_twiss_beta[0] = buffer(cp->m_string->p[0]);

    /* START defining a TWISS input command for the sequence */
    tnl = local_twiss[0]->cmd_def->par_names;
    tpos = name_list_pos("beta0", tnl);
    local_twiss[0]->cmd_def->par_names->inform[tpos] = 1;
    local_twiss[0]->cmd_def->par->parameters[tpos]->string = embedded_twiss_beta[0];
    /* END defining a TWISS input command for the sequence */
  }

  /* END CHK-BETA0; OB 23.1.2002 */

  /* END CHK-RANGE; OB 12.11.2002 */

  /* START CHK-USEORBIT; HG 28.1.2003 */
  pos = name_list_pos("useorbit", nl);
  if (pos > -1 && nl->inform[pos])  /* parameter has been read */
  {
    /* useorbit specified */
    cp = cmd->clone->par->parameters[pos];
    /* START adding useorbit to TWISS input command for each sequence */
    tnl = local_twiss[0]->cmd_def->par_names;
    tpos = name_list_pos("useorbit", tnl);
    local_twiss[0]->cmd_def->par_names->inform[tpos] = 1;
    local_twiss[0]->cmd_def->par->parameters[tpos]->string
      = buffer(cp->m_string->p[0]);
    /* END adding range to TWISS input command for each sequence */
  }
  /* END CHK-USEORBIT; HG 28.1.2003 */

  /* START CHK-KEEPORBIT; HG 28.1.2003 */
  pos = name_list_pos("keeporbit", nl);
  if (pos > -1 && nl->inform[pos])  /* parameter has been read */
  {
    /* keeporbit specified */
    cp = cmd->clone->par->parameters[pos];
    /* START adding keeporbit to TWISS input command for each sequence */
    tnl = local_twiss[0]->cmd_def->par_names;
    tpos = name_list_pos("keeporbit", tnl);
    local_twiss[0]->cmd_def->par_names->inform[tpos] = 1;
    local_twiss[0]->cmd_def->par->parameters[tpos]->string
      = buffer(cp->m_string->p[0]);
    /* END adding range to TWISS input command for each sequence */
  }
  /* END CHK-KEEPORBIT; HG 28.1.2003 */

  /* END CHK-BETA-INPUT; OB 1.2.2002 */

  /* START generating a TWISS table via 'pro_twiss'; OB 1.2.2002 */

  current_global_twiss = current_twiss;
  current_twiss = local_twiss[0]->clone;
  pro_embedded_twiss(current_global_twiss);

  /* END generating a TWISS table via 'pro_twiss' */
  current_twiss = current_global_twiss;

  return 0;
}

struct table* read_table(struct in_cmd* cmd)
  /* reads and stores TFS table */
{
  struct table* t = NULL;
  struct char_p_array* tcpa = NULL;
  struct name_list* tnl = NULL;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("file", nl);
  short sk;
  int i, k, error = 0;
  char *cc, *filename, *type = NULL, *tmp, *name;

  char* namtab;

  if ((namtab = command_par_string("table",cmd->clone)) != NULL) {
    printf("Want to make named table: %s\n",namtab);
  }
  else
  {
    if (get_option("debug")) {
      printf("No table name requested\n");
      printf("Use default name (i.e. name from file) \n");
    }
    namtab = NULL;
  }

  if(nl->inform[pos] && (filename = pl->parameters[pos]->string) != NULL)
  {
    if ((tab_file = fopen(filename, "r")) == NULL)
    {
      fatal_error("cannot open file:", filename); return NULL;
    }
  }
  else
  {
    warning("no filename,","ignored"); return NULL;
  }
  while (fgets(aux_buff->c, aux_buff->max, tab_file))
  {
    supp_char('\r', aux_buff->c);
    cc = strtok(aux_buff->c, " \"\n");
    if (*cc == '@')
    {
      if ((tmp = strtok(NULL, " \"\n")) != NULL
          && strcmp(tmp, "TYPE") == 0)
      {
        if ((name = strtok(NULL, " \"\n")) != NULL) /* skip format */
        {
          if ((name = strtok(NULL, " \"\n")) != NULL)
            type = permbuff(stolower(name));
        }
      }
      else if (strcmp(tmp, "NAME") == 0)
      {
        if ((name = strtok(NULL, " \"\n")) != NULL) /* skip format */
        {
          if ((name = strtok(NULL, " \"\n")) != NULL)
            namtab = permbuff(stolower(name));
        }
      }
    }
    else if (*cc == '*' && tnl == NULL)
    {
      tnl = new_name_list("table_names", 20);
      while ((tmp = strtok(NULL, " \"\n")) != NULL)
        add_to_name_list(permbuff(stolower(tmp)), 0, tnl);
    }
    else if (*cc == '$' && tcpa == NULL)
    {
      if (tnl == NULL)
      {
        warning("formats before names","skipped"); return NULL;
      }
      tcpa = new_char_p_array(20);
      while ((tmp = strtok(NULL, " \"\n")) != NULL)
      {
        if (tcpa->curr == tcpa->max) grow_char_p_array(tcpa);
        if (strcmp(tmp, "%s") == 0)       tnl->inform[tcpa->curr] = 3;
        else if (strcmp(tmp, "%d") == 0)  tnl->inform[tcpa->curr] = 1;
        else if (strcmp(tmp, "%hd") == 0) tnl->inform[tcpa->curr] = 1;
        else                              tnl->inform[tcpa->curr] = 2;
        tcpa->p[tcpa->curr++] = permbuff(tmp);
      }
    }
    else
    {
      if(t == NULL)
      {
        if (type == NULL)
        {
          warning("TFS table without type,","skipped"); error = 1;
        }
        else if (tcpa == NULL)
        {
          warning("TFS table without formats,","skipped"); error = 1;
        }
        else if (tnl == NULL)
        {
          warning("TFS table without column names,","skipped"); error = 1;
        }
        else if (tnl->curr == 0)
        {
          warning("TFS table: empty column name list,","skipped");
          error = 1;
        }
        else if (tnl->curr != tcpa->curr)
        {
          warning("TFS table: number of names and formats differ,",
                  "skipped");
          error = 1;
        }
        if (error)
        {
          delete_name_list(tnl); return NULL;
        }
        if(namtab != NULL) {
          t = new_table(namtab, type,    500, tnl);
        }
        else
        {
          t = new_table(type, type,    500, tnl);
        }
      }
      for (i = 0; i < tnl->curr; i++)
      {
        if (t->curr == t->max) grow_table(t);
        tmp = tcpa->p[i];
        if (strcmp(tmp,"%s") == 0) t->s_cols[i][t->curr] = stolower(tmpbuff(cc));
        else if (strcmp(tmp,"%d") == 0)
        {
          sscanf(cc, tmp, &k); t->d_cols[i][t->curr] = k;
        }
        else if (strcmp(tmp,"%hd") == 0)
        {
          sscanf(cc, tmp, &sk); t->d_cols[i][t->curr] = sk;
        }
        else sscanf(cc, tmp, &t->d_cols[i][t->curr]);
        if (i+1 < tnl->curr)
        {
          if ((cc =strtok(NULL, " \"\n")) == NULL)
          {
            warning("incomplete table line starting with:", aux_buff->c);
            return NULL;
          }
        }
      }
      t->curr++;
    }
  }
  fclose(tab_file);
  if ((tab_file = fopen(filename, "r")) == NULL)
  {
    warning("cannot open file:", filename); return NULL;
  }
/* read & store table header */
  t->header = new_char_p_array(50);
  while (fgets(aux_buff->c, aux_buff->max, tab_file))
  {
    supp_char('\r', aux_buff->c);
    if ((*aux_buff->c != ' ') &&
        ((*aux_buff->c == '@') || (*aux_buff->c == '*')))
    {
      if (t->header->curr == t->header->max) grow_char_p_array(t->header);
      t->header->p[t->header->curr]
        = (char*) mymalloc("read_table", strlen(aux_buff->c)+1);
      strcpy(t->header->p[t->header->curr], aux_buff->c);
      t->header->curr++;
    }
  }
  fclose(tab_file);
  t->origin = 1;
  add_to_table_list(t, table_register);
  return NULL;
}


struct table* read_his_table(struct in_cmd* cmd)
  /* reads and stores TFS table */
{
  struct table* t = NULL;
  struct char_p_array* tcpa = NULL;
  struct name_list* tnl = NULL;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("file", nl);
  int i, k, error = 0;
  char *cc, *filename, *type = NULL, *tmp, *name;

  if(nl->inform[pos] && (filename = pl->parameters[pos]->string) != NULL)
  {
    if ((tab_file = fopen(filename, "r")) == NULL)
    {
      warning("cannot open file:", filename); return NULL;
    }
  }
  else
  {
    warning("no filename,","ignored"); return NULL;
  }
  while (fgets(aux_buff->c, aux_buff->max, tab_file))
  {
    cc = strtok(aux_buff->c, " \"\n");
    if (*cc == '@')
    {
      if ((tmp = strtok(NULL, " \"\n")) != NULL
          && strcmp(tmp, "TYPE") == 0)
      {
        if ((name = strtok(NULL, " \"\n")) != NULL) /* skip format */
        {
          if ((name = strtok(NULL, " \"\n")) != NULL)
            type = permbuff(stolower(name));
        }
      }
    }
    else if (*cc == '*' && tnl == NULL)
    {
      tnl = new_name_list("table_names", 20);
      while ((tmp = strtok(NULL, " \"\n")) != NULL)
        add_to_name_list(permbuff(stolower(tmp)), 0, tnl);
    }
    else if (*cc == '$' && tcpa == NULL)
    {
      if (tnl == NULL)
      {
        warning("formats before names","skipped"); return NULL;
      }
      tcpa = new_char_p_array(20);
      while ((tmp = strtok(NULL, " \"\n")) != NULL)
      {
        if (tcpa->curr == tcpa->max) grow_char_p_array(tcpa);
        if (strcmp(tmp, "%s") == 0)       tnl->inform[tcpa->curr] = 3;
        else if (strcmp(tmp, "%hd") == 0) tnl->inform[tcpa->curr] = 1;
        else                              tnl->inform[tcpa->curr] = 2;
        tcpa->p[tcpa->curr++] = permbuff(tmp);
      }
    }
    else
    {
      if(t == NULL)
      {
        if (type == NULL)
        {
          warning("TFS table without type,","skipped"); error = 1;
        }
        else if (tcpa == NULL)
        {
          warning("TFS table without formats,","skipped"); error = 1;
        }
        else if (tnl == NULL)
        {
          warning("TFS table without column names,","skipped"); error = 1;
        }
        else if (tnl->curr == 0)
        {
          warning("TFS table: empty column name list,","skipped");
          error = 1;
        }
        else if (tnl->curr != tcpa->curr)
        {
          warning("TFS table: number of names and formats differ,",
                  "skipped");
          error = 1;
        }
        if (error)
        {
          delete_name_list(tnl); return NULL;
        }
        t = new_table(type, "input", 500, tnl);
      }
      for (i = 0; i < tnl->curr; i++)
      {
        if (t->curr == t->max) grow_table(t);
        tmp = tcpa->p[i];
        if (strcmp(tmp,"%s") == 0)
          t->s_cols[i][t->curr] = tmpbuff(stolower(cc));
        else if (strcmp(tmp,"%d") == 0 || strcmp(tmp,"%hd") == 0)
        {
          sscanf(cc, tmp, &k); t->d_cols[i][t->curr] = k;
        }
        else sscanf(cc, tmp, &t->d_cols[i][t->curr]);
        if (i+1 < tnl->curr)
        {
          if ((cc =strtok(NULL, " \"\n")) == NULL)
          {
            warning("incomplete table line starting with:", aux_buff->c);
            return NULL;
          }
        }
      }
      t->curr++;
    }
  }
  fclose(tab_file);
  t->origin = 1;
  add_to_table_list(t, table_register);
  return NULL;
}

void remove_from_command_list(char* label, struct command_list* list)
{
  int i;
  if ((i = remove_from_name_list(label, list->list)) > -1)
  {
    if (i < --list->curr)
    {
      delete_command(list->commands[i]);
      list->commands[i] = list->commands[list->curr];
    }
  }
}

void remove_from_node_list(struct node* node, struct node_list* nodes)
{
  int i;
  if ((i = remove_from_name_list(node->name, nodes->list)) > -1)
    nodes->nodes[i] = nodes->nodes[--nodes->curr];
}

int remove_one(struct node* node)
{
  int pos;
  /* removes a node from a sequence being edited */
  if ((pos = name_list_pos(node->p_elem->name, occ_list)) < 0)  return 0;
  if (node->previous != NULL) node->previous->next = node->next;
  if (node->next != NULL) node->next->previous = node->previous;
  if (occ_list->inform[pos] == 1)
  {
    remove_from_node_list(node, edit_sequ->nodes);
    remove_from_name_list(node->p_elem->name, occ_list);
  }
  else --occ_list->inform[pos];
  /* myfree(rout_name, node); */
  return 1;
}

void replace_one(struct node* node, struct element* el)
  /* replaces an existing node by a new one made from el */
{
  int i, k = 1;
  remove_from_node_list(node, edit_sequ->nodes);
  if ((i = name_list_pos(el->name, occ_list)) < 0)
    i = add_to_name_list(el->name, 1, occ_list);
  else k = ++occ_list->inform[i];
  strcpy(node->name, compound(el->name, k));
  add_to_node_list(node, 0, edit_sequ->nodes);
  node->p_elem = el;
  node->base_name = el->base_type->name;
  node->length = el->length;
  if (strcmp(el->base_type->name, "rfcavity") == 0 &&
      find_element(el->name, edit_sequ->cavities) == NULL)
    add_to_el_list(&el, 0, edit_sequ->cavities, 0);
}

void replace_lines(struct macro* org, int replace, char** reps)
  /* replaces lines in line by elements - recursive */
{
  int i, j, k, l, n, pos;
  int mf = replace < org->n_formal ? replace : org->n_formal;
  char* p;
  struct macro* line;
  if (org->tokens == NULL) fatal_error("line not split:", org->name);
  line = clone_macro(org);
  for (j = 0; j < mf; j++)
  {
    for (i = 0; i < line->tokens->curr; i++)
    {
      p = line->tokens->p[i];
      if (isalpha(*p) && strcmp(line->formal->p[j], p) == 0)
        line->tokens->p[i] = reps[j];
    }
  }
  for (i = 0; i < line->tokens->curr; i++)
  {
    p = line->tokens->p[i];
    if (isalpha(*p) && (pos = name_list_pos(p, line_list->list)) > -1)
    {
      if (*line->tokens->p[i+1] == '(') /* formal arguments */
      {
        for (k = i+2; k < line->tokens->curr; k++)
          if (*line->tokens->p[k] == ')') break;
        n = k - i - 2;
        l = k;
      }
      else
      {
        n = 0; l = i;
      }
      replace_lines(line_list->macros[pos], n, &line->tokens->p[i+2]);
      i = l;
    }
    else
    {
      if (line_buffer->curr == line_buffer->max)
        grow_char_p_array(line_buffer);
      line_buffer->p[line_buffer->curr++] = tmpbuff(p);
    }
  }
  delete_macro(line);
}

void reset_count(char* table) /* resets table counter to zero */
{
  int pos;
  struct table* t;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return;
  t->curr = 0;
}

void reset_errors(struct sequence* sequ)
  /* zeros the sel_err node flag for all nodes of an expanded sequence */
{
  struct node* c_node;
  if (sequ != NULL && sequ->ex_start != NULL && sequ->ex_end != NULL)
  {
    c_node = sequ->ex_start;
    while (c_node != NULL)
    {
      c_node->sel_err = 0;
      if (c_node == sequ->ex_end) break;
      c_node = c_node->next;
    }
  }
}

int reset_interpolation(int *nint)
{
  struct node *c_node, *second_node;
  int j,bend_flag = 0;
  double angle,length,e1,e2,numint, h1, h2, fint, fintx, hgap, bvk;

  /* Deletes the interpolating nodes expanded by the routine interp_node */

  numint = (double)*nint;

  /* reset first and last node in the sequence range */

  current_sequ->range_start = current_sequ->ex_start;
  current_sequ->range_end = current_sequ->ex_end;

  /* reset current_node at first node */

  for (j = 1; j <= *nint ; j++)
    current_node = current_node->previous;

  /* reset length of first node */

  length = numint*current_node->length;
  current_node->length = length;

  /* resets angle and saves e1 if the element is a bending magnet */

  bend_flag = strcmp(current_node->p_elem->base_type->name, "sbend") == 0 || rbend;
  if (bend_flag)
  {
    angle = numint*node_value("angle");
    store_node_value("angle",&angle);
    e1 = node_value("e1");
    h1 = node_value("h1");
    fint = node_value("fint");
    fintx = fintx_plot;
    hgap = node_value("hgap");
  }

  /* advance to nint-th  node (second node in original sequence) */

  for (j = 1; j <= *nint; j++)
    advance_node();
  second_node = current_node;

  /* back to the last interpolated node */

  retreat_node();

  /* saves e2 if the element is a bending magnet */

  if (bend_flag)
  {
    e2 = node_value("e2");
    h2 = node_value("h2");
  }

  /* delete the interpolating nodes */

  for (j = 2; j <= *nint; j++)
  {
    c_node = current_node;

    retreat_node();
    if (bend_flag)
    {
      c_node->p_elem->def = delete_command(c_node->p_elem->def);
      c_node->p_elem = delete_element(c_node->p_elem);
    }
    delete_node(c_node);
  }

  /* current_node points now to the first node of the original sequence */
  /* sets next pointer of first node to second node of original sequence */

  current_node->next = second_node;

  /* sets pointer of second node to first node of original sequence */

  current_node->next->previous = current_node;

  /* Updates the values of e1 and e2 and stores them in first node */

/*  bv = node_value("dipole_bv");*/
  bvk = node_value("other_bv");

  if (bend_flag)
  {
    if (rbend)
    {
      strcpy(current_node->p_elem->base_type->name,"rbend");
      current_node->p_elem->def->mad8_type = 2;
      e1 = e1 - bvk * angle / two;
      e2 = e2 - bvk * angle / two;
    }
    store_node_value("e1",&e1);
    store_node_value("e2",&e2);
    store_node_value("h1",&h1);
    store_node_value("h2",&h2);
    store_node_value("fint",&fint);
    store_node_value("fintx",&fintx_plot);
    store_node_value("hgap",&hgap);
  }

  return 0;
}

void reset_sector(struct sequence* sequ, int val)
  /* sets node->sel_sector = val for all nodes of an expanded sequence */
{
  struct node* c_node;
  if (sequ != NULL && sequ->ex_start != NULL && sequ->ex_end != NULL)
  {
    c_node = sequ->ex_start;
    while (c_node != NULL)
    {
      c_node->sel_sector = val;
      if (c_node == sequ->ex_end) break;
      c_node = c_node->next;
    }
  }
}

int restart_sequ()
{
  current_node = current_sequ->range_start;
  return 1;
}

int retreat_node()
  /* replaces current node by previous node; 0 = already at start, else 1 */
{
  if (current_node == current_sequ->range_start)  return 0;
  current_node = current_node->previous;
  return 1;
}

double rfc_slope()
  /* calculates the accumulated "slope" of all cavities */
{
  double slope = zero, lag, volt, harmon, charge, pc;
  struct node* c_node = current_sequ->range_start;
  struct element* el;
  charge = command_par_value("charge", current_beam);
  pc = command_par_value("pc", current_beam);
  do
  {
    el = c_node->p_elem;
    if (strcmp(el->base_type->name, "rfcavity") == 0 &&
        (harmon = command_par_value("harmon", el->def)) > zero)
    {
      volt = command_par_value("volt", el->def);
      lag = command_par_value("lag", el->def);
      slope += ten_m_3 * charge * volt * harmon * cos(twopi * lag) / pc;
    }
    if (c_node == current_sequ->range_end) break;
    c_node = c_node->next;
  }
  while (c_node != NULL);
  return slope;
}

void sector_out(char* sector_table_name, double* pos, double* kick,
                double* rmatrix, double* tmatrix)
{
  int i;
  int j;
  int k;
  int index;

  /* the name is not \0 finished and displays ugly in C */
  /* but well, this is how table names are handled... */

  char * elementName = current_node->p_elem->name;

  string_to_table( sector_table_name, "name", elementName );
  double_to_table( sector_table_name, "pos", pos );

  /* 6 kicks */
  for (i=0; i<6; i++){
    char kickStr[2+1];
    sprintf(kickStr,"k%i",i+1);
    kickStr[2]='\0';
    index = i;
    double_to_table( sector_table_name, kickStr,&kick[index]);
  }
  /* 36 R-matrix terms */
  for (j=0; j<6; j++){
    for (i=0; i<6; i++){
      char rStr[3+1];
      sprintf(rStr,"r%i%i",i+1,j+1);
      rStr[3]='\0';
      index = i+j*6;
      double_to_table( sector_table_name, rStr,&rmatrix[index]);
    }
  }
  /* 216 T-matrix terms */
  for (k=0; k<6; k++){
    for (j=0; j<6; j++){
      for (i=0;i<6; i++){
	char tStr[4+1];
	sprintf(tStr,"t%i%i%i",i+1,j+1,k+1);
	tStr[4]='\0';
	index = i+j*6+k*36;
	double_to_table( sector_table_name, tStr,&tmatrix[index]);
      }
    }
  }

  augment_count( sector_table_name ); /* move to next record */
}

void seq_cycle(struct in_cmd* cmd)
  /* cycles a sequence */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct node *node, *clone;
  char* name = NULL;
  int pos = name_list_pos("start", nl);
  if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
  {
    sprintf(c_dum->c, "%s:1", name);
    if ((pos = name_list_pos(c_dum->c, edit_sequ->nodes->list)) > -1)
    {
      node = edit_sequ->nodes->nodes[pos];
      sprintf(c_dum->c, "%s_p_",strip(node->name));
      if (strstr(node->previous->name, "_p_") == NULL)
      {
        clone = clone_node(node, 0);
        clone->p_elem = clone_element(node->p_elem);
        strcpy(clone->p_elem->name, c_dum->c);

        /* IA 29.11.07 : fixes a bug with aperture module*/
        sprintf(c_dum->c, " ");
        set_command_par_string("apertype", clone->p_elem->def,c_dum->c);


        add_to_el_list(&clone->p_elem, node->p_elem->def->mad8_type,
                       element_list, 1);
        link_in_front(clone, node);
      }
      edit_sequ->start = node;
      edit_sequ->end = node->previous;
      set_new_position(edit_sequ);
      all_node_pos(edit_sequ);
    }
    else warning("cycle: unknown element ignored:", name);
  }
  else warning("cycle: no start given,","ignored");
}

void seq_edit_main(struct in_cmd* cmd)
  /* controls sequence editing */
{
  int k = cmd->decl_start - 1;
  char** toks = cmd->tok_list->p;
  if (strcmp(toks[k], "seqedit") == 0)  seq_edit(cmd);
  else if(edit_is_on)
  {
    if (strcmp(toks[k], "install") == 0)  seq_install(cmd);
    else if (strcmp(toks[k], "move") == 0)  seq_move(cmd);
    else if (strcmp(toks[k], "remove") == 0)  seq_remove(cmd);
    else if (strcmp(toks[k], "cycle") == 0)  seq_cycle(cmd);
    else if (strcmp(toks[k], "flatten") == 0)  seq_flatten(edit_sequ);
    else if (strcmp(toks[k], "reflect") == 0)  seq_reflect(cmd);
    else if (strcmp(toks[k], "replace") == 0)  seq_replace(cmd);
    else if (strcmp(toks[k], "endedit") == 0)  seq_end(cmd);
  }
  else warning("seqedit command outside edit", "ignored");
}

void seq_edit(struct in_cmd* cmd)
  /* executes seqedit command */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  char* name = NULL;
  int pos;
  pos = name_list_pos("sequence", nl);
  if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
  {
    if ((pos = name_list_pos(name, sequences->list)) >= 0)
    {
      if (sequences->sequs[pos]->line)
        warning("sequence originates from line,","edit ignored");
      else  seq_edit_ex(sequences->sequs[pos]);
    }
    else warning("unknown sequence:", "ignored");
  }
  else warning("seqedit without sequence:", "ignored");
}

void seq_end(struct in_cmd* cmd)
  /* executes endedit command */
{
  char tmp[8];
  sprintf(tmp, "%d", seqedit_install);
  put_info("seqedit - number of elements installed: ", tmp);
  sprintf(tmp, "%d", seqedit_move);
  put_info("seqedit - number of elements moved:     ", tmp);
  sprintf(tmp, "%d", seqedit_remove);
  put_info("seqedit - number of elements removed:   ", tmp);
  seq_end_ex();
}

void seq_install(struct in_cmd* cmd)
  /* executes install command */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct element *cl, *el;
  struct node* c_node;
  struct expression* expr = NULL;
  double at, from = zero;
  char name[NAME_L], *pname, *name_e = NULL, *name_c = NULL, *from_name = NULL;
  int k, pos, any = 0;
  int pos_e = name_list_pos("element", nl);
  int pos_c = name_list_pos("class", nl);
  if (nl->inform[pos_e] && (name_e = pl->parameters[pos_e]->string) != NULL)
  {
    if (nl->inform[pos_c] && (name_c = pl->parameters[pos_c]->string) != NULL)
    {
      if ((cl = find_element(name_c, element_list)) == NULL)
      {
        warning("ignored because of unknown class:", name_c);
        return;
      }
      else
      {
        el = clone_element(cl);
        strcpy(el->name, name_e);
        add_to_el_list(&el, cl->def->mad8_type, element_list, 2);
      }
    }
    else if ((el = find_element(name_e, element_list)) == NULL)
    {
      warning("ignored, unknown command or element:", name_c); return;
    }
  }
  else
  {
    warning("no element specified,","ignored"); return;
  }
  if (nl->inform[name_list_pos("at", nl)] == 0)
  {
    warning("no 'at':", "ignored"); return;
  }
  at = command_par_value("at", cmd->clone);
  expr = clone_expression(command_par_expr("at", cmd->clone));
  pos = name_list_pos("from", nl);
  if (nl->inform[pos])
  {
    from_name = pl->parameters[pos]->string;
    if (strcmp(from_name, "selected") == 0)
    {
      if (seqedit_select->curr == 0)
      {
        warning("no active select commands:", "ignored"); return;
      }
      else
      {
        if (get_select_ranges(edit_sequ, seqedit_select, selected_ranges)
            == 0) any = 1;
        c_node = edit_sequ->start;
        while (c_node != NULL)
        {
          if (any
              || name_list_pos(c_node->name, selected_ranges->list) > -1)
          {
            for (k = 0; k < seqedit_select->curr; k++)
            {
              myrepl(":", "[", c_node->name, name);
              strcat(name, "]");
              if (strchr(name, '$') == NULL &&
                  pass_select(c_node->name,
                              seqedit_select->commands[k])) break;
            }
            if (k < seqedit_select->curr)
            {
              from = get_node_pos(c_node, edit_sequ);
              pname = permbuff(name);
              install_one(el, pname, at, expr, at+from);
              seqedit_install++;
            }
          }
          if (c_node == edit_sequ->end) break;
          c_node = c_node->next;
        }
      }
    }
    else
    {
      from_name = permbuff(pl->parameters[pos]->string);
      if ((from = hidden_node_pos(from_name, edit_sequ)) == INVALID)
      {
        warning("ignoring 'from' reference to unknown element:", from_name);
        return;
      }
      install_one(el, from_name, at, expr, at+from);
      seqedit_install++;
    }
  }
  else
  {
    install_one(el, from_name, at, expr, at);
    seqedit_install++;
  }
}

void seq_move(struct in_cmd* cmd)
  /* executes move command */
{
  char *name, *from_name;
  double at, by, to, from = zero;
  int any = 0, k;
  struct node *node, *next;
  struct element* el;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos = name_list_pos("element", nl);
  if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
  {
    if (strcmp(name, "selected") == 0)
    {
      if (seqedit_select->curr == 0)
      {
        warning("no active select commands:", "ignored"); return;
      }
      else
      {
        if (nl->inform[name_list_pos("by", nl)] == 0)
        {
          warning("no 'by' given,", "ignored"); return;
        }
        by = command_par_value("by", cmd->clone);
        if (get_select_ranges(edit_sequ, seqedit_select, selected_ranges)
            == 0) any = 1;
        node = edit_sequ->start;
        while (node != edit_sequ->end)
        {
          node = node->next; node->moved = 0;
        }
        node = edit_sequ->start;
        while (node != NULL && node != edit_sequ->end)
        {
          next = node->next;
          if (node->moved == 0)
          {
            if (any
                || name_list_pos(node->name, selected_ranges->list) > -1)
            {
              name = NULL;
              for (k = 0; k < seqedit_select->curr; k++)
              {
                if (node->p_elem != NULL) name = node->p_elem->name;
                if (name != NULL && strchr(name, '$') == NULL &&
                    pass_select(name,
                                seqedit_select->commands[k])) break;
              }
              if (k < seqedit_select->curr)
              {
                at = node->position + by;
                el = node->p_elem;
                if (remove_one(node) > 0)
                {
                  node = install_one(el, NULL, at, NULL, at);
                  node->moved = 1;
                  seqedit_move++;
                }
              }
            }
          }
          node = next;
        }
      }
    }
    else
    {
      strcpy(c_dum->c, name);
      square_to_colon(c_dum->c);
      if ((pos = name_list_pos(c_dum->c, edit_sequ->nodes->list)) > -1)
      {
        node = edit_sequ->nodes->nodes[pos];
        if (nl->inform[name_list_pos("by", nl)] == 0)
        {
          if (nl->inform[name_list_pos("to", nl)] == 0)
          {
            warning("no position given,", "ignored"); return;
          }
          to = command_par_value("to", cmd->clone);
          pos = name_list_pos("from", nl);
          if (nl->inform[pos])
          {
            from_name = pl->parameters[pos]->string;
            if ((from = hidden_node_pos(from_name, edit_sequ)) == INVALID)
            {
              warning("ignoring 'from' reference to unknown element:",
                      from_name);
              return;
            }
          }
          at = to + from;
        }
        else
        {
          by = command_par_value("by", cmd->clone);
          at = node->position + by;
        }
        el = node->p_elem;
        if (remove_one(node) > 0)
        {
          install_one(el, NULL, at, NULL, at);
          seqedit_move++;
        }
      }
    }
  }
}

void seq_reflect(struct in_cmd* cmd)
  /* executes reflect command */
{
  struct node *tmp, *c_node;
  c_node = edit_sequ->start;
  while (c_node != NULL)
  {
    tmp = c_node->next;
    c_node->next = c_node->previous;
    c_node->previous = tmp;
    if (c_node == edit_sequ->end) break;
    c_node = tmp;
  }
  tmp = edit_sequ->start;
  edit_sequ->start = edit_sequ->end;
  edit_sequ->end = tmp;
  c_node = edit_sequ->start;
  edit_sequ->range_start = edit_sequ->start;
  edit_sequ->range_end = edit_sequ->end;
  while (c_node != NULL)
  {
    c_node->at_expr = NULL;
    c_node->from_name = NULL;
    c_node->position = c_node->at_value
      = sequence_length(edit_sequ) - c_node->position;
    if (c_node == edit_sequ->end) break;
    c_node = c_node->next;
  }
}

void seq_remove(struct in_cmd* cmd)
  /* executes remove command */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct node *c_node;
  char *name;
  int k, any = 0;
  int pose = name_list_pos("element", nl);
  if (nl->inform[pose] && (name = pl->parameters[pose]->string) != NULL)
  {
    if (strcmp(name, "selected") == 0)
    {
      if (seqedit_select->curr == 0)
      {
        warning("no active select commands:", "ignored"); return;
      }
      else
      {
        if (get_select_ranges(edit_sequ, seqedit_select, selected_ranges)
            == 0) any = 1;
        c_node = edit_sequ->start;
        while (c_node != NULL)
        {
          if (any
              || name_list_pos(c_node->name, selected_ranges->list) > -1)
          {
            name = NULL;
            for (k = 0; k < seqedit_select->curr; k++)
            {
              if (c_node->p_elem != NULL) name = c_node->p_elem->name;
              if (name != NULL && strchr(name, '$') == NULL &&
                  pass_select(name,
                              seqedit_select->commands[k])) break;
            }
            if (k < seqedit_select->curr)
            {
              seqedit_remove += remove_one(c_node);
            }
          }
          if (c_node == edit_sequ->end) break;
          c_node = c_node->next;
        }
      }
    }
    else
    {
      strcpy(c_dum->c, name);
      square_to_colon(c_dum->c);
      if ((pose = name_list_pos(c_dum->c, edit_sequ->nodes->list)) > -1)
      {
        seqedit_remove += remove_one(edit_sequ->nodes->nodes[pose]);
      }
      else warning("ignored because of unknown element:", name);
    }
  }
  else  warning("no element specified,","ignored");
}

void seq_replace(struct in_cmd* cmd)
  /* executes replace command */
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct node** rep_nodes = NULL;
  struct element** rep_els = NULL;
  struct node *node, *c_node;
  char* name;
  struct element* el;
  int count = count_nodes(edit_sequ);
  int any = 0, k, rep_cnt = 0, pos = name_list_pos("element", nl);
  if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
  {
    if (strcmp(name, "selected") == 0)
    {
      if (seqedit_select->curr == 0)
      {
        warning("no active select commands:", "ignored"); return;
      }
      else
      {
        pos = name_list_pos("by", nl);
        if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
        {
          if ((el = find_element(name, element_list)) == NULL)
          {
            warning("ignoring unknown 'by' element:",name);
            return;
          }
        }
        else
        {
          warning("'by' missing, ","ignored");
          return;
        }
        rep_nodes = (struct node**)
          mymalloc("seq_replace", count*sizeof(struct node*));
        rep_els = (struct element**)
          mymalloc("seq_replace", count*sizeof(struct element*));
        if (get_select_ranges(edit_sequ, seqedit_select, selected_ranges)
            == 0) any = 1;
        c_node = edit_sequ->start;
        while (c_node != NULL)
        {
          if (any || name_list_pos(c_node->name, selected_ranges->list) > -1)
          {
            name = NULL;
            for (k = 0; k < seqedit_select->curr; k++)
            {
              if (c_node->p_elem != NULL) name = c_node->p_elem->name;
              if (name != NULL && strchr(name, '$') == NULL &&
                  pass_select(name,
                              seqedit_select->commands[k])) break;
            }
            if (k < seqedit_select->curr)
            {
              rep_els[rep_cnt] = el;
              rep_nodes[rep_cnt++] = c_node;
            }
          }
          if (c_node == edit_sequ->end) break;
          c_node = c_node->next;
        }
      }
    }
    else
    {
      rep_nodes = (struct node**)
        mymalloc("seq_replace", count*sizeof(struct node*));
      rep_els = (struct element**)
        mymalloc("seq_replace", count*sizeof(struct element*));
      strcpy(c_dum->c, name);
      square_to_colon(c_dum->c);
      if ((pos = name_list_pos(c_dum->c, edit_sequ->nodes->list)) > -1)
      {
        node = edit_sequ->nodes->nodes[pos];
        pos = name_list_pos("by", nl);
        if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
        {
          if ((el = find_element(name, element_list)) != NULL)
          {
            rep_els[rep_cnt] = el;
            rep_nodes[rep_cnt++] = node;
          }
          else warning("ignoring unknown 'by' element: ",name);
        }
        else warning("'by' missing, ","ignored");
      }
      else warning("ignored because of unknown element: ", name);
    }
    for (k = 0; k < rep_cnt; k++)  replace_one(rep_nodes[k], rep_els[k]);
    if (rep_nodes) myfree("seq_replace", rep_nodes);
    if (rep_els)   myfree("seq_replace", rep_els);
  }
  else  warning("no element specified, ","ignored");
}

void sequence_name(char* name, int* l)
  /* returns current sequence name in Fortran format */
{
  int sname_l = strlen(current_sequ->name);
  int i, ncp = sname_l < *l ? sname_l : *l;
  int nbl = *l - ncp;
  for (i = 0; i < ncp; i++) name[i] = current_sequ->name[i];
  for (i = 0; i < nbl; i++) name[ncp+i] = ' ';
}

void set_new_position(struct sequence* sequ)
  /* sets a new node position for all nodes */
{
  struct node* c_node = sequ->start;
  double zero_pos = c_node->position;
  int flag = 0;
  while (c_node != NULL)
  {
    if (c_node->from_name == NULL)
    {
      c_node->position -= zero_pos;
      if (c_node->position < zero || (flag && c_node->position == zero))
        c_node->position += sequence_length(sequ);
      if (c_node->position > zero) flag = 1;
      c_node->at_value = c_node->position;
      c_node->at_expr = NULL;
    }
    if (c_node == sequ->end) break;
    c_node = c_node->next;
  }
  c_node->position = c_node->at_value = sequence_length(sequ);
}

void set_node_bv(struct sequence* sequ)
  /* sets bv flag for all nodes */
{
  struct node* c_node = sequ->ex_start;
  double beam_bv;
  beam_bv = command_par_value("bv", current_beam);
  while (c_node != NULL)
  {
    c_node->other_bv = beam_bv;
    c_node->dipole_bv = beam_bv;
/* dipole_bv kill initiative SF TR FS */
/*      if (c_node->p_elem->bv) c_node->dipole_bv = beam_bv;
        else                    c_node->dipole_bv = 1;*/
    if (c_node == sequ->ex_end) break;
    c_node = c_node->next;
  }
}

void set_value(char* name, char* par, double* value)
  /* sets parameter value "par" for command or store "name" if present */
{
  mycpy(c_dum->c, name);
  mycpy(aux_buff->c, par);
  if (strcmp(c_dum->c, "beam") == 0)
    set_command_par_value(aux_buff->c, current_beam, *value);
  else if (strcmp(c_dum->c, "probe") == 0)
    set_command_par_value(aux_buff->c, probe_beam, *value);
  else if (strcmp(c_dum->c, "survey") == 0)
    set_command_par_value(aux_buff->c, current_survey, *value);
  else if (strcmp(c_dum->c, "twiss") == 0)
    set_command_par_value(aux_buff->c, current_twiss, *value);
  else if (current_command != NULL
           && strcmp(c_dum->c, current_command->name) == 0)
    set_command_par_value(aux_buff->c, current_command, *value);
}

double sss_variable(char* name)
{
  char comm[NAME_L];
  char par[NAME_L];
  double val = zero;
  struct variable* var;
  struct element* el;
  struct command* cmd;
  char *p, *n = c_dum->c, *q = comm;
  mycpy(c_dum->c, name);
  if ((p = strstr(c_dum->c, "->")) == NULL) /* variable */
  {
    if ((var = find_variable(c_dum->c, variable_list)) != NULL)
      val = variable_value(var);
  }
  else /* element or command parameter */
  {
    while (n < p)  *(q++) = *(n++);
    *q = '\0';
    q = par; n++; n++;
    while (*n != '\0')  *(q++) = *(n++);
    *q = '\0';
    if ((el = find_element(comm, element_list)) != NULL)
      val = command_par_value(par, el->def);
    else if ((cmd = find_command(comm, stored_commands)) != NULL)
      val = command_par_value(par, cmd);
    else if ((cmd = find_command(comm, beta0_list)) != NULL)
      val = command_par_value(par, cmd);
    else if ((cmd = find_command(comm, defined_commands)) != NULL)
      val = command_par_value(par, cmd);
  }
  return val;
}

int set_enable(char* type, struct in_cmd* cmd)
{
  char* name;
  struct command_parameter* cp;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct sequence* sequ;
  struct node* nodes[2];
  struct node* c_node;
  int pos, k, n, status, count = 0;
  pos = name_list_pos("sequence", nl);
  if(nl->inform[pos]) /* sequence specified */
  {
    cp = cmd->clone->par->parameters[pos];
    if ((n = name_list_pos(cp->string, sequences->list)) >= 0)
      sequ = sequences->sequs[n];
    else
    {
      warning(cp->string," :sequence not found, skipped");
      return 0;
    }
  }
  else sequ = current_sequ;
  if (sequ->ex_start == NULL)
  {
    warning(sequ->name," :sequence not USEed, skipped");
    return 0;
  }
  pos = name_list_pos("status", nl);
  if (pos > -1 && nl->inform[pos])  /* parameter has been read */
  {
    name = pl->parameters[pos]->string;
    status = strcmp(name, "on") == 0 ? 1 : 0;
  }
  else status = 1;
  pos = name_list_pos("range", nl);
  if (pos > -1 && nl->inform[pos])  /* parameter has been read */
  {
    name = pl->parameters[pos]->string;
    if ((k = get_ex_range(name, sequ, nodes)) == 0)
    {
      nodes[0] = NULL; nodes[1] = NULL;
    }
  }
  else
  {
    nodes[0] = sequ->ex_start; nodes[1] = sequ->ex_end;
  }
  c_node = nodes[0];
  while (c_node)
  {
    if (strstr(c_node->base_name, type) &&
        pass_select(c_node->p_elem->name, cmd->clone) != 0)
    {
      c_node->enable = status; count++;
    }
    if (c_node == nodes[1]) break;
    c_node = c_node->next;
  }
  return count;
}

void set_range(char* range, struct sequence* sequ)
{
  struct node* nodes[2];
  current_sequ->range_start = current_sequ->ex_start;
  current_sequ->range_end = current_sequ->ex_end;
  if (get_ex_range(range, sequ, nodes) == 0) return;
  current_sequ->range_start = nodes[0];
  current_sequ->range_end = nodes[1];
}

void set_selected_columns(struct table* t, struct command_list* select)
{
  int i, j, pos, k, n = 0;
  char* p;
  struct name_list* nl;
  struct command_parameter_list* pl;
  if (select && par_present("column", NULL, select))
  {
    for (j = 0; j < t->num_cols; j++)  /* deselect all columns */
      t->col_out->i[j] = 0;
    t->col_out->curr = 0;
    for (i = 0; i < select->curr; i++)
    {
      nl = select->commands[i]->par_names;
      pl = select->commands[i]->par;
      pos = name_list_pos("column", nl);
      if (nl->inform[pos])
      {
        for (j = 0; j < pl->parameters[pos]->m_string->curr; j++)
        {
          if (strcmp(pl->parameters[pos]->m_string->p[j], "re") == 0)
          {
            for (k = 0; k < t->num_cols; k++)
            {
              if (strncmp("re", t->columns->names[k], 2) == 0)
              {
                if (k <  t->num_cols
                    && int_in_array(k, n, t->col_out->i) == 0)
                  t->col_out->i[n++] = k;
              }
            }
          }
          else if (strcmp(pl->parameters[pos]->m_string->p[j], "eign") == 0)
          {
            for (k = 0; k < t->num_cols; k++)
            {
              if (strncmp("eign", t->columns->names[k], 2) == 0)
              {
                if (k <  t->num_cols
                    && int_in_array(k, n, t->col_out->i) == 0)
                  t->col_out->i[n++] = k;
              }
            }
          }
          else if (strcmp(pl->parameters[pos]->m_string->p[j],
                          "apertype") == 0)
          {
            for (k = 0; k < t->num_cols; k++)
            {
              if (strncmp("aper", t->columns->names[k], 4) == 0)
              {
                if (k <  t->num_cols
                    && int_in_array(k, n, t->col_out->i) == 0)
                  t->col_out->i[n++] = k;
              }
            }
          }
          else
          {
            p = pl->parameters[pos]->m_string->p[j];
            if ((k = name_list_pos(p, t->columns)) > -1)
            {
              if (k <  t->num_cols
                  && int_in_array(k, n, t->col_out->i) == 0)
                t->col_out->i[n++] = k;
            }
          }
        }
      }
    }
    t->col_out->curr = n;
  }
}

void set_selected_errors()
{
  int i, flag;
  if ((flag =
       get_select_ex_ranges(current_sequ, error_select, selected_ranges)) != 0)
  {
    for (i = 0; i < selected_ranges->curr; i++)
      selected_ranges->nodes[i]->sel_err = 1;
  }
}

void set_selected_rows(struct table* t, struct command_list* select,
                       struct command_list* deselect)
{
  int i, j;
  c_range_start = get_node_count(current_sequ->range_start);
  c_range_end = get_node_count(current_sequ->range_end);
  get_select_t_ranges(select, deselect, t);
  if (select != 0)
  {
    for (j = 0; j < t->curr; j++)  t->row_out->i[j] = 0;
    for (i = 0; i < select->curr; i++)
    {
      for (j = s_range->i[i]; j <= e_range->i[i]; j++)
      {
        if (t->row_out->i[j] == 0) t->row_out->i[j]
                                     = pass_select(t->s_cols[0][j], select->commands[i]);
      }
    }
  }
  if (deselect != NULL)
  {
    for (i = 0; i < deselect->curr; i++)
    {
      for (j = sd_range->i[i]; j <= ed_range->i[i]; j++)
      {
        if (t->row_out->i[j] == 1) t->row_out->i[j]
                                     = 1 - pass_select(t->s_cols[0][j], deselect->commands[i]);
      }
    }
  }
}

void set_twiss_deltas(struct command* comm)
{
  char* string;
  int i, k = 0, n = 0, pos;
  double s, sign = one, ar[3];
  struct name_list* nl = comm->par_names;
  pos = name_list_pos("deltap", nl);
  twiss_deltas->curr = 1;
  twiss_deltas->a[0] = zero;
  if ((pos = name_list_pos("deltap", nl)) >= 0 && nl->inform[pos]
      && (string = comm->par->parameters[pos]->string) != NULL)
  {
    pre_split(string, c_dum, 0);
    mysplit(c_dum->c, tmp_p_array);
    while (k < tmp_p_array->curr)
    {
      for (i = k; i < tmp_p_array->curr; i++)
        if (*tmp_p_array->p[i] == ':') break;
      ar[n++] = double_from_expr(tmp_p_array->p, k, i-1);
      k = i + 1;
    }
    if (n == 1) twiss_deltas->a[0] = ar[0];
    else  /* there is a range given - fill array */
    {
      if (n == 2) ar[n++] = ar[1] - ar[0];
      if (ar[2] == zero) twiss_deltas->a[0] = ar[0];
      else if (ar[2] * (ar[1] - ar[0]) < zero)
        warning("illegal deltap range ignored:", string);
      else
      {
        twiss_deltas->a[0] = ar[0];
        if (ar[2] < zero) sign = -sign;
        for (s = sign * (ar[0] + ar[2]);
             s <= sign * ar[1]; s+= sign * ar[2])
        {
          if (twiss_deltas->curr == twiss_deltas->max)
          {
            sprintf(c_dum->c, "%d values", twiss_deltas->max);
            warning("deltap loop cut at", c_dum->c);
            break;
          }
          twiss_deltas->a[twiss_deltas->curr]
            = twiss_deltas->a[twiss_deltas->curr-1] + ar[2];
          twiss_deltas->curr++;
        }
      }
    }
  }
}

void set_sector()
{
  int i, flag;
  if (sector_select->curr == 0) reset_sector(current_sequ, 1);
  else
  {
    sector_ranges->curr = 0; sector_ranges->list->curr = 0;
    if ((flag =
         get_select_ex_ranges(current_sequ, sector_select, sector_ranges)) != 0)
    {
      for (i = 0; i < sector_ranges->curr; i++)
        sector_ranges->nodes[i]->sel_sector = 1;
    }
  }
}

void store_beta0(struct in_cmd* cmd)
{
  int k = cmd->decl_start - 1;
  if (k == 0) warning("beta0 without label:", "ignored");
  else
  {
    cmd->clone_flag = 1; /* do not delete */
    add_to_command_list(cmd->tok_list->p[0], cmd->clone, beta0_list, 0);
  }
}

void store_comm_par_vector(char* parameter, double* val, struct command* cmd)
{
  struct command_parameter* cp;
  int i;
  if ((i = name_list_pos(parameter, cmd->par_names)) > -1)
  {
    cp = cmd->par->parameters[i];
    if (cp->double_array != NULL)
    {
      copy_double(val, cp->double_array->a, cp->double_array->curr);
      if (cp->expr_list != NULL)
        cp->expr_list = delete_expr_list(cp->expr_list);
    }
  }
}

void store_deselect(struct in_cmd* cmd)
{
  char* flag_name;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct command_list* dscl;
  int pos = name_list_pos("flag", nl);
  if (nl->inform[pos] == 0 ||
      (flag_name = pl->parameters[pos]->string) == NULL)
  {
    warning("no FLAG specified", "ignored");
    return;
  }
  if (strcmp(flag_name, "seqedit") == 0)
  {
  }
  else if (strcmp(flag_name, "error") == 0)
  {
  }
  else if (strcmp(flag_name, "makethin") == 0)
  {
  }
  else if (strcmp(flag_name, "save") == 0)
  {
  }
  else if (strcmp(flag_name, "sectormap") == 0)
  {
  }
  else /* store deselect for all tables */
  {
    if ((dscl = find_command_list(flag_name, table_deselect)) == NULL)
    {
      dscl = new_command_list("deselect", 10);
      add_to_command_list_list(flag_name, dscl, table_deselect);
    }
    if (log_val("clear", cmd->clone))
    {
      dscl = new_command_list("deselect", 10);
      add_to_command_list_list(flag_name, dscl, table_deselect);
    }
    else
    {
      if (dscl->curr == dscl->max) grow_command_list(dscl);
      dscl->commands[dscl->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
}

void store_savebeta(struct in_cmd* cmd)
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos;
  char* name = NULL;
  struct command* comm;
  if (log_val("clear", cmd->clone))
  {
    delete_command_list(savebeta_list);
    savebeta_list = new_command_list("savebeta_list", 10);
    delete_command_list(beta0_list);
    beta0_list = new_command_list("beta0_list", 10);
  }
  else if (nl->inform[name_list_pos("place", nl)] == 0)
    warning("savebeta without place:", "ignored");
  else
  {
    pos = name_list_pos("label", nl);
    if (nl->inform[pos])  name = pl->parameters[pos]->string;
    else warning("savebeta without label:", "ignored");
    if (name != NULL)
    {
      cmd->clone_flag = 1; /* do not delete */
      if ((comm = find_command(name, beta0_list)))
        remove_from_command_list(name, beta0_list);
      add_to_command_list(permbuff(name), cmd->clone, savebeta_list, 0);
    }
  }
}

void store_select(struct in_cmd* cmd)
{
#ifdef _ONLINE
  char *sdds_pattern;
  char sdum[1000];
#endif
  char* flag_name;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct command_list* scl;
  int pos = name_list_pos("flag", nl);
  if (nl->inform[pos] == 0 ||
      (flag_name = pl->parameters[pos]->string) == NULL)
  {
    warning("no FLAG specified", "ignored");
    return;
  }
  if (strcmp(flag_name, "seqedit") == 0)
  {
    if (log_val("clear", cmd->clone))
    {
      delete_command_list(seqedit_select);
      seqedit_select = new_command_list("seqedit_select", 10);
      selected_ranges->curr = 0;
      selected_ranges->list->curr = 0;
    }
    else
    {
      if (seqedit_select->curr == seqedit_select->max)
        grow_command_list(seqedit_select);
      seqedit_select->commands[seqedit_select->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
  else if (strcmp(flag_name, "error") == 0)
  {
    if (log_val("clear", cmd->clone))
    {
      delete_command_list(error_select);
      error_select = new_command_list("error_select", 10);
      selected_ranges->curr = 0;
      selected_ranges->list->curr = 0;
      reset_errors(current_sequ);
    }
    else
    {
      if (error_select->curr == error_select->max)
        grow_command_list(error_select);
      error_select->commands[error_select->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
  else if (strcmp(flag_name, "sdds") == 0)
#ifdef _ONLINE
  {
    if (log_val("clear", cmd->clone))
    {
      if (get_option("debug")) printf("here CLEAR all selected arrays\n");
      if(sdds_pat != NULL) sdds_pat = delete_char_p_array(sdds_pat, 1);
      if (get_option("debug")) printf("ptr after clear: %d\n",(long)sdds_pat);
    }
    else
    {
      if(sdds_pat == NULL) sdds_pat = new_char_p_array(100);
      if (get_option("debug")) printf("current selections %d\n",sdds_pat->curr);
      sdds_pattern = command_par_string("pattern",cmd->clone);
      if(sdds_pattern != NULL) {
        if (get_option("debug")) printf("pattern in select: %s\n",sdds_pattern);
        strcpy(sdum,sdds_pattern);
        sdds_pat->p[sdds_pat->curr++] = tmpbuff(sdum);
      }
    }
  }
#else
  {
    warning("ignored, only available in ONLINE model:", "FLAG=SDDS");
  }
#endif

  else if (strcmp(flag_name, "makethin") == 0)
  {
    if (log_val("clear", cmd->clone))
    {
      slice_select->curr = 0;
    }
    else
    {
      if (slice_select->curr == slice_select->max)
        grow_command_list(slice_select);
      slice_select->commands[slice_select->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
  else if (strcmp(flag_name, "save") == 0)
  {
    if (log_val("clear", cmd->clone))
    {
      save_select->curr = 0;
    }
    else
    {
      if (save_select->curr == save_select->max)
        grow_command_list(save_select);
      save_select->commands[save_select->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
  else if (strcmp(flag_name, "sectormap") == 0)
  {
    if (sector_ranges == NULL)   sector_ranges = new_node_list(10000);
    if (log_val("clear", cmd->clone))
    {
      delete_command_list(sector_select);
      sector_select = new_command_list("sector_select", 10);
      sector_ranges->curr = 0;
      sector_ranges->list->curr = 0;
    }
    else
    {
      if (sector_select->curr == sector_select->max)
        grow_command_list(sector_select);
      sector_select->commands[sector_select->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
  else /* store select for all tables */
  {
    if ((scl = find_command_list(flag_name, table_select)) == NULL)
    {
      scl = new_command_list("select", 10);
      add_to_command_list_list(flag_name, scl, table_select);
    }
    if (log_val("clear", cmd->clone))
    {
      scl = new_command_list("select", 10);
      add_to_command_list_list(flag_name, scl, table_select);
    }
    else
    {
      if (scl->curr == scl->max) grow_command_list(scl);
      scl->commands[scl->curr++] = cmd->clone;
      cmd->clone_flag = 1; /* do not drop */
    }
  }
}

void store_node_vector(char* par, int* length, double* vector)
  /* stores vector at node */
{
  char lpar[NAME_L];
  mycpy(lpar, par);
  if (strcmp(lpar, "orbit0") == 0)  copy_double(vector, orbit0, 6);
  else if (strcmp(lpar, "orbit_ref") == 0)
  {
    if (current_node->orbit_ref)
    {
      while (*length > current_node->orbit_ref->max)
        grow_double_array(current_node->orbit_ref);
    }
    else current_node->orbit_ref = new_double_array(*length);
    copy_double(vector, current_node->orbit_ref->a, *length);
    current_node->orbit_ref->curr = *length;
  }
}

void store_orbit(struct command* comm, double* orbit)
{
  struct name_list* nl = comm->par_names;
  if (nl->inform[name_list_pos("x", nl)])
    orbit[0] = command_par_value("x",comm);
  if (nl->inform[name_list_pos("px", nl)])
    orbit[1] = command_par_value("px",comm);
  if (nl->inform[name_list_pos("y", nl)])
    orbit[2] = command_par_value("y",comm);
  if (nl->inform[name_list_pos("py", nl)])
    orbit[3] = command_par_value("py",comm);
  if (nl->inform[name_list_pos("t", nl)])
    orbit[4] = command_par_value("t",comm);
  if (nl->inform[name_list_pos("pt", nl)])
    orbit[5] = command_par_value("pt",comm);
}

void store_threader(struct in_cmd* cmd)
{
  threader_par = cmd->clone;
  cmd->clone_flag = 1;
  dump_command(threader_par);
}

void string_to_table(char* table, char* name, char* string)
  /* buffers + puts "string"
     at current position in column with name "name".
     The table count is increased separately with "augment_count" */
{
  int pos;
  struct table* t;

  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return;
  mycpy(c_dum->c, name);
  if ((pos = name_list_pos(c_dum->c, t->columns)) >= 0
      && t->columns->inform[pos] == 3)
  {
    mycpy(c_dum->c, string);
    if (strcmp(c_dum->c, "name") == 0)
      t->s_cols[pos][t->curr] = tmpbuff(current_node->name);
    else t->s_cols[pos][t->curr] = tmpbuff(c_dum->c);
  }
}

int table_length(char* table)
  /* returns no. of rows in table */
{
  int pos;
  int length = 0;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    length = table_register->tables[pos]->curr;
  return length;
}

int table_org(char* table)
  /* returns origin: 0  this job, 1 read or unknown */
{
  int pos;
  int org = 1;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    org = table_register->tables[pos]->origin;
  return org;
}

void table_range(char* table, char* range, int* rows)
  /* returns first and last row numbers (start=1) in rows
     or 0 if table or range invalid */
{
  int pos;
  struct table* t;

  rows[0] = rows[1] = 0;
  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
  {
    t = table_register->tables[pos];
    get_table_range(range, t, rows);
    rows[0]++; rows[1]++;
  }
}


void track_dynap(struct in_cmd* cmd)
{
  char rout_name[] = "track_dynap";
  int e_flag, flag = 2, izero = 0,
    turns = command_par_value("turns", cmd->clone),
    npart = 2*stored_track_start->curr;
  int *ibuf1, *ibuf2, *ibuf3;
  double orbit[6];
  double *buf1, *buf2, *buf_dxt, *buf_dyt, *buf3, *buf4, *buf5, *buf6,
    *buf7, *buf8, *buf9, *buf10, *buf11;
  struct table* t;
  int kopt01,kopt02;


  kopt02=0;
  kopt01 = get_value("dynap","damp");
  if (kopt01 == 0) {
    kopt02=1;
    fprintf(prt_file, "damp is on\n");}
  set_option("damp", &kopt02);



  kopt02=0;
  kopt01 = get_value("dynap","quantum");
  if (kopt01 == 0) {
    kopt02=1;
    fprintf(prt_file, "quantum is on\n");}
  set_option("quantum", &kopt02);




  if (track_is_on == 0)
  {
    warning("track_dynap: no TRACK command seen yet", "ignored");
    return;
  }
  if (npart == 0)
  {
    warning("track_dynap: no START command seen yet", "ignored");
    return;
  }
  if (turns < 64)
  {
    warning("track_dynap: turns cannot be < 64", "set to 64");
    turns = 64;
  }
  adjust_beam();
  if (probe_beam) probe_beam = delete_command(probe_beam);
  probe_beam = clone_command(current_beam);
  adjust_probe(track_deltap); /* sets correct gamma, beta, etc. */
  adjust_rfc(); /* sets freq in rf-cavities from probe */
  zero_double(orbit0, 6);
  zero_double(oneturnmat, 36);
  if (get_option("onepass") == 0)
  {
    tmrefo_(&izero,orbit0,orbit,oneturnmat);
    /* closed orbit and one-turn linear transfer map */
  }
  dynap_tables_create(cmd);
  /* allocate buffers */
  ibuf1 = (int*) mymalloc(rout_name,npart*sizeof(int));
  ibuf2 = (int*) mymalloc(rout_name,npart*sizeof(int));
  ibuf3 = (int*) mymalloc(rout_name, current_sequ->n_nodes*sizeof(int));
  buf1 = (double*) mymalloc(rout_name,npart*sizeof(double));
  buf2 = (double*) mymalloc(rout_name,6*npart*sizeof(double));
  buf_dxt = (double*) mymalloc(rout_name,npart*sizeof(double));
  buf_dyt = (double*) mymalloc(rout_name,npart*sizeof(double));
  buf3 = (double*) mymalloc(rout_name,6*npart*sizeof(double));
  buf4 = (double*) mymalloc(rout_name,36*sizeof(double)); /* eigenvectors */
  buf5 = (double*) mymalloc(rout_name,6*npart*(turns+1)*sizeof(double));
  buf6 = (double*) mymalloc(rout_name, current_sequ->n_nodes*sizeof(double));
  buf7 = (double*) mymalloc(rout_name, turns*sizeof(double));
  buf8 = (double*) mymalloc(rout_name, 6*turns*sizeof(double));
  buf9 = (double*) mymalloc(rout_name, 2*turns*sizeof(double));
  buf10 = (double*) mymalloc(rout_name, turns*sizeof(double));
  buf11 = (double*) mymalloc(rout_name, turns*sizeof(double));
  trrun_(&flag, &turns,orbit0, oneturnmat, ibuf1, ibuf2, buf1, buf2,
         buf_dxt, buf_dyt, buf3, buf4, buf5, &e_flag, ibuf3, buf6);
  t =
    table_register->tables[name_list_pos("tracksumm", table_register->names)];
  print_table(t);
  if (e_flag)
  {
    warning("track_dynap: particle lost before last turn,", "ignored");
    return;
  }
  dynap_(buf4, buf5, &turns, &npart, buf7, buf8, buf9, buf10, buf11);
  /*
    table_register->tables[name_list_pos("dynapsumm", table_register->names)];
    print_table(t);
    if (get_option("dynap_dump")) dynap_tables_dump();
  */
  /* free buffers */
  myfree(rout_name, ibuf1); myfree(rout_name, ibuf2);
  myfree(rout_name, ibuf3); myfree(rout_name, buf1); myfree(rout_name, buf2);
  myfree(rout_name, buf_dxt); myfree(rout_name, buf_dyt);
  myfree(rout_name, buf3); myfree(rout_name, buf4); myfree(rout_name, buf5);
  myfree(rout_name, buf6); myfree(rout_name, buf7); myfree(rout_name, buf8);
  myfree(rout_name, buf9); myfree(rout_name, buf10);
  myfree(rout_name, buf11);
}

void track_end(struct in_cmd* cmd)
{
  int i;
  struct node* c_node;
  if (track_is_on == 0)
  {
    warning("track_end: no TRACK command seen yet", "ignored");
    return;
  }
  for (i = 0; i < stored_track_start->curr; i++)
    stored_track_start->commands[i] =
      delete_command(stored_track_start->commands[i]);
  stored_track_start->curr = 0;
  c_node = current_sequ->ex_start;
  while(c_node != NULL) /* clean observation points */
  {
    c_node->obs_point = 0;
    c_node->obs_orbit = delete_double_array(c_node->obs_orbit);
    if (c_node == current_sequ->ex_end)  break;
    c_node = c_node->next;
  }
  track_is_on = 0;
  fprintf(prt_file, "exit TRACK module\n\n");
}

void ptc_track_end()
{
  int i;
  struct node* c_node;
  if (track_is_on == 0)
  {
    warning("ptc_track_end: no PTC_TRACK command seen yet", "ignored");
    return;
  }
  for (i = 0; i < stored_track_start->curr; i++)
    stored_track_start->commands[i] =
      delete_command(stored_track_start->commands[i]);
  stored_track_start->curr = 0;
  c_node = current_sequ->ex_start;
  while(c_node != NULL) /* clean observation points */
  {
    c_node->obs_point = 0;
    c_node->obs_orbit = delete_double_array(c_node->obs_orbit);
    if (c_node == current_sequ->ex_end)  break;
    c_node = c_node->next;
  }
  curr_obs_points = 1;
  track_is_on = 0;
}

void track_observe(struct in_cmd* cmd)
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct node* nodes[2];
  int pos;
  if (track_is_on == 0)
  {
    warning("track_observe: no TRACK command seen yet,", "ignored");
    return;
  }
  pos = name_list_pos("place", nl);
  if (get_ex_range(pl->parameters[pos]->string, current_sequ, nodes))
  {
    nodes[0]->obs_point = ++curr_obs_points;
    nodes[0]->obs_orbit = new_double_array(6);
    nodes[0]->obs_orbit->curr = 6;
    adjust_beam();
    if (probe_beam) probe_beam = delete_command(probe_beam);
    probe_beam = clone_command(current_beam);
    adjust_probe(track_deltap); /* sets correct gamma, beta, etc. */
    adjust_rfc(); /* sets freq in rf-cavities from probe */
    zero_double(orbit0, 6);
    zero_double(oneturnmat, 36);
    if (get_option("onepass") == 0)
    {
      tmrefo_(&curr_obs_points,orbit0,nodes[0]->obs_orbit->a,oneturnmat);
      /* closed orbit and one-turn linear transfer map */
    }
  }
  else
  {
    warning("track_observe: unknown place,", "ignored");
    return;
  }
}

void ptc_track_observe(struct in_cmd* cmd)
{
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  struct node* nodes[2];
  int pos;

  pos = name_list_pos("place", nl);
  if (get_ex_range(pl->parameters[pos]->string, current_sequ, nodes))
  {
    nodes[0]->obs_point = ++curr_obs_points;
    printf("obs_points: %d \n",curr_obs_points);
  }
  else
  {
    warning("ptc_track_observe: unknown place,", "ignored");
    return;
  }
}

void track_pteigen(double* eigen)
{
  int i, j, pos;
  struct table* t;
  double tmp;
  if ((pos = name_list_pos("trackone", table_register->names)) > -1)
  {
    t = table_register->tables[pos];
    if (t->header == NULL)  t->header = new_char_p_array(45);
    sprintf(c_dum->c, v_format("@ XC               %%le  %F"), orbit0[0]);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    sprintf(c_dum->c, v_format("@ PXC              %%le  %F"), orbit0[1]);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    sprintf(c_dum->c, v_format("@ YC               %%le  %F"), orbit0[2]);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    sprintf(c_dum->c, v_format("@ PYC              %%le  %F"), orbit0[3]);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    sprintf(c_dum->c, v_format("@ TC               %%le  %F"), orbit0[4]);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    sprintf(c_dum->c, v_format("@ PTC              %%le  %F"), orbit0[5]);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    tmp = get_value("beam", "ex");
    sprintf(c_dum->c, v_format("@ EX               %%le  %F"), tmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    tmp = get_value("beam", "ey");
    sprintf(c_dum->c, v_format("@ EY               %%le  %F"), tmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    tmp = get_value("beam", "et");
    sprintf(c_dum->c, v_format("@ ET               %%le  %F"), tmp);
    t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
    for (i = 0; i < 6; i++)
    {
      for (j = 0; j < 6; j++)
      {
        sprintf(c_dum->c, v_format("@ E%d%d              %%le  %F"),
                i+1, j+1, eigen[6*j+i]);
        t->header->p[t->header->curr++] = tmpbuff(c_dum->c);
      }
    }
  }
}

void track_run(struct in_cmd* cmd)
{
  char rout_name[] = "track_run";
  int e_flag, flag = 1, izero = 0, npart = stored_track_start->curr;
  int *ibuf1, *ibuf2, *ibuf3;
  double orbit[6];
  double d_dummy, *buf1, *buf2, *buf_dxt, *buf_dyt, *buf3, *buf4, *buf5,
    *buf6;
  struct table* t;
  int turns = command_par_value("turns", cmd->clone);
  if (track_is_on == 0)
  {
    warning("track_run: no TRACK command seen yet", "ignored");
    return;
  }
  if (npart == 0)
  {
    warning("track_run: no START command seen yet", "ignored");
    return;
  }
  adjust_beam();
  if (probe_beam) probe_beam = delete_command(probe_beam);
  probe_beam = clone_command(current_beam);
  adjust_probe(track_deltap); /* sets correct gamma, beta, etc. */
  adjust_rfc(); /* sets freq in rf-cavities from probe */
  zero_double(orbit0, 6);
  zero_double(oneturnmat, 36);
  if (get_option("onepass") == 0)
  {
    tmrefo_(&izero,orbit0,orbit,oneturnmat);
    /* closed orbit and one-turn linear transfer map */
  }
  track_tables_create(cmd);
  /* allocate buffers */
  ibuf1 = (int*) mymalloc(rout_name,npart*sizeof(int));
  ibuf2 = (int*) mymalloc(rout_name,npart*sizeof(int));
  ibuf3 = (int*) mymalloc(rout_name,current_sequ->n_nodes*sizeof(int));
  buf1 = (double*) mymalloc(rout_name,npart*sizeof(double));
  buf2 = (double*) mymalloc(rout_name,6*npart*sizeof(double));
  buf_dxt = (double*) mymalloc(rout_name,npart*sizeof(double));
  buf_dyt = (double*) mymalloc(rout_name,npart*sizeof(double));
  buf3 = (double*) mymalloc(rout_name,6*npart*sizeof(double));
  buf4 = (double*) mymalloc(rout_name,36*sizeof(double));
  buf5 = &d_dummy;
  buf6 = (double*) mymalloc(rout_name, current_sequ->n_nodes*sizeof(double));
  trrun_(&flag, &turns,orbit0, oneturnmat, ibuf1, ibuf2, buf1, buf2,
         buf_dxt, buf_dyt, buf3, buf4, buf5, &e_flag, ibuf3, buf6);
  t =
    table_register->tables[name_list_pos("tracksumm", table_register->names)];
  if (get_option("info"))  print_table(t);
  if (get_option("track_dump")) track_tables_dump();
  /* free buffers */
  myfree(rout_name, ibuf1); myfree(rout_name, ibuf2); myfree(rout_name, ibuf3);
  myfree(rout_name, buf1); myfree(rout_name, buf2);
  myfree(rout_name, buf_dxt); myfree(rout_name, buf_dyt);
  myfree(rout_name, buf3);
  myfree(rout_name, buf4); myfree(rout_name, buf6);
  fprintf(prt_file, "\n*****  end of trrun  *****\n");
}
/********************************************************************************/
void ptc_dumpmaps(struct in_cmd* cmd)
/*Dumps PTC map for each element in the current sequence*/
{
  w_ptc_dumpmaps_();
}
/********************************************************************************/
void ptc_oneturnmap(struct in_cmd* cmd)
{
}
/********************************************************************************/

void pro_ptc_trackline(struct in_cmd* cmd)
{
  /*Does PTC tracking taking to the account acceleration */
  /*it is basically wrapper to subroutine ptc_trackline() in madx_ptc_trackline.f90*/

  int pos, one;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;

  pos = name_list_pos("file", nl);

  if (nl->inform[pos])
  {
    set_option("track_dump", &one);
  }

  if ((track_filename = pl->parameters[pos]->string) == NULL)
  {
    if (pl->parameters[pos]->call_def != NULL)
    {
      track_filename = pl->parameters[pos]->call_def->string;
    }
    else
    {
      track_filename = permbuff("dummy");
    }
  }
  track_filename = permbuff(track_filename);
  track_fileext = NULL;
  pos = name_list_pos("extension", nl);

  if ((track_fileext = pl->parameters[pos]->string) == NULL)
  {
    if (pl->parameters[pos]->call_def != NULL)
    {
      track_fileext = pl->parameters[pos]->call_def->string;
    }
    if (track_fileext == NULL)
    {
      track_fileext = permbuff("\0");
    }
  }

  track_fileext = permbuff(track_fileext);

  if (command_par_value("everystep",cmd->clone) != 0)
  {
    printf("Enforcing onetable=true, current is %f\n", command_par_value("onetable",cmd->clone));
    set_command_par_value("onetable", cmd->clone, 1.0);
    printf("Now is %f\n", command_par_value("onetable",cmd->clone));
  }

  track_tables_create(cmd);


  if (command_par_value("everystep",cmd->clone) != 0)
  {
    printf("Calling PTC track line every step\n");
    w_ptc_track_everystep(&curr_obs_points);
  }
  else
  {
    printf("Calling STD PTC track line\n");
    w_ptc_trackline(&curr_obs_points);
  }

  track_tables_dump();
}
/********************************************************************************/

void pro_ptc_enforce6d(struct in_cmd* cmd)
{
  /*Does PTC tracking taking to the account acceleration */
  /*it is basically wrapper to subroutine ptc_trackline() in madx_ptc_trackline.f90*/
  double switchvalue;
  struct name_list* nl;
  int flag;

  if (cmd == 0x0)
  {
    warning("pro_ptc_enforce6d:","Command is null!!!");
    return;
  }

  if (cmd->clone == 0x0)
  {
    error("pro_ptc_enforce6d","Command Definintion is null!!!");
    return;
  }

  nl = cmd->clone->par_names;

  /*DEBUG LEVEL SWITCH*/
  if ( name_list_pos("flag", nl) >=0 )
  {
    command_par_value2("flag", cmd->clone, &switchvalue);
    flag = (int)switchvalue;
    w_ptc_enforce6d(&flag);
  }
  else
  {
    printf("flag is not present\n");
  }
}
/********************************************************************************/

void pro_ptc_setswitch(struct in_cmd* cmd)
{
  /*Does PTC tracking taking to the account acceleration */
  /*it is basically wrapper to subroutine ptc_trackline() in madx_ptc_trackline.f90*/
  int i;
  double switchvalue;
  struct name_list* nl;

  if (cmd == 0x0)
  {
    warning("pro_ptc_setswitch:","Command is null!!!");
    return;
  }

  if (cmd->clone == 0x0)
  {
    printf("pro_ptc_setswitch: Command Definintion is null!!!\n");
    return;
  }

  if (match_is_on == kMatch_PTCknobs)
  {
    madx_mpk_setsetswitch(cmd);
    return;
  }

  nl = cmd->clone->par_names;

  /*DEBUG LEVEL SWITCH*/
  if ( name_list_pos("debuglevel", nl) >=0 )
  {
    command_par_value2("debuglevel", cmd->clone, &switchvalue);
    debuglevel = (int)switchvalue;
    w_ptc_setdebuglevel(&debuglevel);
  }
  else
  {
    printf("debuglevel is not present\n");
  }


  /*ACCELERATION SWITCH*/
  if ( name_list_pos("maxacceleration", nl) >=0 )
  {
    command_par_value2("maxacceleration", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("maxaccel is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_setaccel_method(&i);
  }
  else
  {
    if (debuglevel > 0) printf("maxaccel is not present\n");
  }


  /*EXACT SWITCH*/
  if ( name_list_pos("exact_mis", nl) >=0 )
  {
    command_par_value2("exact_mis", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("exact_mis is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_setexactmis(&i);
  }
  else
  {
    if (debuglevel > 0)  printf("exact_mis is not present\n");
  }


  /*radiation SWITCH*/
  if ( name_list_pos("radiation", nl) >=0 )
  {
    command_par_value2("radiation", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("radiation is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_setradiation(&i);
  }
  else
  {
    if (debuglevel > 0) printf("radiation is not present\n");
  }

  /*fringe SWITCH*/
  if ( name_list_pos("fringe", nl) >=0 )
  {
    command_par_value2("fringe", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("fringe is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_setfringe(&i);
  }
  else
  {
    if (debuglevel > 0) printf("fringe is not present\n");
  }



  /*totalpath SWITCH*/
  if ( name_list_pos("totalpath", nl) >=0 )
  {
    command_par_value2("totalpath", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("totalpath is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_settotalpath(&i);
  }
  else
  {
    if (debuglevel > 0) printf("totalpath is not present\n");
  }


  /*TIME SWITCH*/
  if ( name_list_pos("time", nl) >=0 )
  {
    command_par_value2("time", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("time is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_settime(&i);
  }
  else
  {
    if (debuglevel > 0) printf("time is not present\n");
  }

  /*NOCAVITY SWITCH*/
  if ( name_list_pos("nocavity", nl) >=0 )
  {
    command_par_value2("nocavity", cmd->clone, &switchvalue);
    if (debuglevel > 0) printf("nocavity is found and its value is %f\n", switchvalue);
    i = (int)switchvalue;
    w_ptc_setnocavity(&i);
  }
  else
  {
    if (debuglevel > 0) printf("nocavity is not present\n");
  }

  if (debuglevel > 0) printf("obs_points pro_ptc_setswitch Done\n");


}
/********************************************************************************/

void pro_ptc_printparametric(struct in_cmd* cmd)
{
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;

  char*                          filename    = 0x0;
  struct int_array*              filenameIA      = 0x0;
  static int                     zeroint = 0;
  int*                           filep = 0x0;

  pos   = name_list_pos("filename", c_parnames);
  if (pos < 0)
  {

    filep = &zeroint;
  }
  else
  {
    filename  = c_parameters->parameters[pos]->string;
    if ( filename == 0x0 )
    {
      filep = &zeroint;
    }
    else
    {
      filenameIA = new_int_array(1+strlen(filename));
      conv_char(filename,filenameIA);
      filep = filenameIA->i;
    }
  }

  pos   = name_list_pos("format", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_printparametric: format parameter does not exist.\n");
    return;
  }

  w_ptc_writeparresults(filep);

  delete_int_array(filenameIA);


}
/********************************************************************************/

void pro_ptc_printframes(struct in_cmd* cmd)
{
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;

  char*                          filename    = 0x0;
  struct int_array*              filenameIA      = 0x0;
  char*                          format    = 0x0;

  pos   = name_list_pos("file", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_printframes: file parameter does not exist.\n");
    return;
  }

  filename  = c_parameters->parameters[pos]->string;
  if ( filename == 0x0 )
  {
    warning("madxn.c: pro_ptc_printframes: no file name: ", "ignored");
    return;
  }


  pos   = name_list_pos("format", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_printframes: format parameter does not exist.\n");
    return;
  }

  format  = c_parameters->parameters[pos]->string;
  printf("madxn.c: pro_ptc_printframes: format is %s.\n", format);

  filenameIA = new_int_array(1+strlen(filename));

  conv_char(filename,filenameIA);


  if (strcmp(format,"rootmacro") == 0)
  {
    w_ptc_printlayout_rootm(filenameIA->i);
  }
  else
  {
    w_ptc_printframes(filenameIA->i);
  }

  delete_int_array(filenameIA);


}

/********************************************************************************/

void pro_ptc_export_xml(struct in_cmd* cmd)
{
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;
  char*                          filename    = 0x0;
  struct int_array*              filenameIA      = 0x0;
/*  char*                          format    = 0x0; */

  pos   = name_list_pos("file", c_parnames);
  if (pos < 0)
  {
    /* should never enter here */
    printf("madxn.c: pro_ptc_export_xml: file parameter does not exist.\n");
    return;
  }

  filename  = c_parameters->parameters[pos]->string;
  printf("will write to file %s\n",filename);

  if ( filename == 0x0 )
  {
    warning("madxn.c: pro_ptc_export_xml: no file name: ", "ignored");
    return;
  }

  filenameIA = new_int_array(1+strlen(filename));

  conv_char(filename,filenameIA);

  w_ptc_export_xml(filenameIA->i);

  delete_int_array(filenameIA);

}



/********************************************************************************/


void pro_ptc_eplacement(struct in_cmd* cmd)
{/*
   Sets a parameter
 */
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;
  int                            k         = 0;
  struct node*                   nodes[2]={0x0,0x0};
  struct node*                   anode=0x0;
  char*                          element;
  int                            refframe=0;/*0 global, 1 current position, 2 end face if the previous element*/


  pos   = name_list_pos("refframe", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_eplacement: refframe parameter does not exist.\n");
    return;
  }

  if ( c_parnames->inform[pos] != 0 )
  {
    /*if it is zero it is not specified*/

    if ( c_parameters->parameters[pos]->string == 0x0 )
    {
      warning("madxn.c: pro_ptc_eplacement: string describing refframe is null: ", "using default");
      refframe = 0;
    }
    else
    {
      /*printf("refframe is %s.\n", c_parameters->parameters[pos]->string );*/

      if ( strcmp(c_parameters->parameters[pos]->string,"current")  == 0 )
      {
        refframe = 1;
      }

      if ( strcmp(c_parameters->parameters[pos]->string,"previouselement") == 0 )
      {
        refframe = 2;
      }
    }
  }


  pos   = name_list_pos("range", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_eplacement: range parameter does not exist.\n");
    return;
  }

  if ( c_parnames->inform[pos] == 0 )
  {
    printf("madxn.c: pro_ptc_eplacement: inform for range is 0.\n");
    return;
  }

  element  = c_parameters->parameters[pos]->string;
  if ( element == 0x0 )
  {
    warning("madxn.c: pro_ptc_eplacement: no element name: ", "ignored");
    return;
  }


  k = get_ex_range(element, current_sequ, nodes);
  if ( k != 1)
  {
    if (k > 1)
    {
      warningnew("pro_ptc_eplacement","More then one element correstponds to the range <<%s>>.",element);
      seterrorflag(1,"pro_ptc_eplacement","More then one element correstponds to the range");
      return;
    }
    else
    {
      warningnew("pro_ptc_eplacement","Element <<%s>> not found",element);
      seterrorflag(1,"pro_ptc_eplacement","Element not found");
      return;
    }
  }



  pos = 0;
  anode=current_sequ->range_start;
  while(anode)
  {
    /*printf("%d: Comparing %#x %s with  %#x %s \n",pos, nodes[0], nodes[0]->name, anode, anode->name);*/
    if ( nodes[0]  == anode  )
    {
      /*printf("Element is at pos %d !\n",pos);*/
      break;
    }

    if (anode == current_sequ->range_end)
    {
      warningnew("pro_ptc_eplacement","Reached the end of sequence - Element <<%s>> not found",element);
      return;
    }

    anode = anode->next;
    pos++; /*before if because fortran numerates from 1*/

  }


  w_ptc_eplacement(&pos,&refframe);


}
/********************************************************************************/

void pro_ptc_varyknob(struct in_cmd* cmd)
{/*
   Sets a variable based on parameter
 */

  if (match_is_on != kMatch_PTCknobs)
  {
    warningnew("pro_ptc_varyknob","Match with ptcknobs is not active, command ignored");
    return;
  }

  madx_mpk_addvariable(cmd);

}
/********************************************************************************/

void pro_ptc_knob(struct in_cmd* cmd)
{/*
   Sets a parameter
 */
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;

  char*                          element     = 0x0;
  struct int_array*              elementIA   = 0x0;
  char*                          initialp    = 0x0;
  struct int_array*              initialpIA  = 0x0;
  char*                                   p  = 0x0;

  pos   = name_list_pos("element", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_knob: element parameter does not exist.\n");
    return;
  }
  element  = c_parameters->parameters[pos]->string;

  pos   = name_list_pos("initial", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_knob: initial parameter does not exist.\n");
    return;
  }

  initialp  = c_parameters->parameters[pos]->string;

  if ( (element == 0x0) && (initialp == 0x0) )
  {
    warning("madxn.c: pro_ptc_knob: no element name neither initial pareter specified: ",
            "command ignored");
    return;
  }

  if (initialp)
  {
    mycpy(c_dum->c, initialp);

    stolower(c_dum->c);

    initialpIA = new_int_array(1+strlen(c_dum->c));

    conv_char(c_dum->c,initialpIA);

    w_ptc_addknob_i(initialpIA->i);

    delete_int_array(initialpIA);

  }
  else
  {
    mycpy(c_dum->c, element);

    if (command_par_value("exactmatch",cmd->clone) != 0)
    {
      p = strstr(c_dum->c,"[");
      if (p)
      {
        *p = ':';
        p = strstr(c_dum->c,"]");
        if (p == 0x0)
        {
          warningnew("madxn.c: pro_ptc_knob:","element %s is bady defned. Commnad ignored.",element);
          return;
        }
        *p=0;
      }
      else
      { /*assume it is the first element*/
        p = &(c_dum->c[strlen(c_dum->c)]);
        p[0]=':';
        p[1]='1';
        p[2]= 0;
      }

    }
    stoupper(c_dum->c);

    elementIA = new_int_array(1+strlen(c_dum->c));

    conv_char(c_dum->c,elementIA);

    w_ptc_addknob(elementIA->i);

    delete_int_array(elementIA);
  }
}
/********************************************************************************/

void pro_ptc_setknobvalue(struct in_cmd* cmd)
{/*
   Sets a parameter value
 */
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;

  char*                          element    = 0x0;
  struct int_array*              elementIA      = 0x0;

  pos   = name_list_pos("element", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_knob: element parameter does not exist.\n");
    return;
  }

  element  = c_parameters->parameters[pos]->string;
  if ( element == 0x0 )
  {
    warning("madxn.c: pro_ptc_knob: no element name: ", "ignored");
    return;
  }
  mycpy(c_dum->c, element);

  stoupper(c_dum->c);

  elementIA = new_int_array(1+strlen(c_dum->c));

  conv_char(c_dum->c,elementIA);

  w_ptc_setknobvalue(elementIA->i);

  delete_int_array(elementIA);


}
/********************************************************************************/


void pro_ptc_setfieldcomp(struct in_cmd* cmd)
{/*
   Sets a parameter value
 */
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;
  int                            k         = 0;
  struct node*                   nodes[2]={0x0,0x0};
  struct node*                   anode=0x0;
  char*                          element;



  pos   = name_list_pos("element", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_setfieldcomp: range parameter does not exist.\n");
    return;
  }

  if ( c_parnames->inform[pos] == 0 )
  {
    printf("madxn.c: pro_ptc_setfieldcomp: inform for range is 0.\n");
    return;
  }

  element  = c_parameters->parameters[pos]->string;
  if ( element == 0x0 )
  {
    warning("madxn.c: pro_ptc_setfieldcomp: no element name: ", "ignored");
    return;
  }


  k = get_range(element, current_sequ, nodes);
  if ( k != 1)
  {
    if (k > 1)
    {
      warningnew("pro_ptc_setfieldcomp","More then one element correstponds to the range <<%s>>.",element);
      seterrorflag(1,"pro_ptc_setfieldcomp","More then one element correstponds to the range");
      return;
    }
    else
    {
      warningnew("pro_ptc_setfieldcomp","Element <<%s>> not found",element);
      seterrorflag(1,"pro_ptc_setfieldcomp","Element not found");
      return;
    }
  }



  pos = 0;
  anode=current_sequ->range_start;
  while( anode != 0x0  )
  {
    if ( nodes[0]  == current_sequ->nodes->nodes[pos]  )
    {
      /* printf("Element is at pos %d !\n",pos);*/
      break;
    }

    if (current_sequ->nodes->nodes[pos] == current_sequ->range_end)
    {
      warningnew("pro_ptc_setfieldcomp","Reached the end of sequence - Element <<%s>> not found",element);
      return;
    }
    pos++; /*before if because fortran numerates from 1*/
  }

  w_ptc_setfieldcomp(&pos);

}
/********************************************************************************/

void pro_ptc_select(struct in_cmd* cmd)
{/*
   processes ptc_select command
   it directs ptc_twiss to store given QUANTITY in a named TABLE's COLUMN
   Then, it these values are accessible for other MAD-X modules for calculations.
   The most important one is the matching module.
 */

  char*                          monomial    = 0x0;

  int                            element     = 0;
  int*                           tablep      = 0;
  int*                           columnp     = 0;
  static int                     zeroint     = 0;/*if there is no column name or table name these are passed as null strings */
  struct int_array*              tabnameIA   = 0x0;/*string passing to fortran is tricky*/
  struct int_array*              colnameIA   = 0x0;/*and is done via integer arrays*/
  struct int_array*              monoIA      = 0x0;

  monomial = command_par_string("monomial",cmd->clone);

  if (monomial == 0x0)
  {
    warning("madxn.c: pro_ptc_select: monomial is NULL ", "ignored");
    return;
  }

  monoIA = new_int_array(1+strlen(monomial));
  conv_char(monomial,monoIA);

  element = (int)command_par_value("polynomial",cmd->clone);

  pro_ptc_select_checkpushtable(cmd,&tabnameIA,&colnameIA);

  if ( tabnameIA )
  {
    tablep = tabnameIA->i;
  }
  else
  {
    tablep = &zeroint;
  }

  if ( colnameIA )
  {
    columnp = colnameIA->i;
  }
  else
  {
    columnp = &zeroint;
  }

  w_ptc_addpush_(tablep,columnp,&element,monoIA->i);

  delete_int_array(tabnameIA);
  delete_int_array(colnameIA);
  delete_int_array(monoIA);

}
/********************************************************************************/

int pro_ptc_select_checkpushtable(struct in_cmd* cmd,
                                  struct int_array** tabnameIA, struct int_array** colnameIA)
{
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  struct table*                  aTable      = 0x0;
  int                            pos         = 0;
  char*                          tablename   = 0x0;
  char*                          columnname  = 0x0;


  /*extracts column specified by the user*/
  pos        = name_list_pos("column", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_select: column parameter does not exist.\n");
    return 5;
  }

  columnname  = c_parameters->parameters[pos]->string;
  if ( columnname == 0x0 )
  {
/*    warning("madxn.c: pro_ptc_select: Column name is empty: ", "ignored");*/
    return 6;
  }

  *colnameIA = new_int_array(1+strlen(columnname));
  conv_char(columnname,*colnameIA);


  /*extracts table specified by the user*/
  pos   = name_list_pos("table", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_select: table parameter does not exist.\n");
    return 1;
  }

  tablename  = c_parameters->parameters[pos]->string;
  if ( tablename == 0x0 )
  {
    return -1;/*This means that table name was not specified at all*/
  }
  if ( tablename[0] == 0 )
  {
    return -1; /*This means that table name was not specified at all*/
  }
  pos = name_list_pos(tablename, table_register->names);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_select: table <<%s>> does not exist: Create table first\n",tablename);
    return 3;
  }

  aTable = table_register->tables[pos];
  if (aTable == 0x0)
  {
    printf("madxn.c: pro_ptc_select: table <<%s>> is NULL: \n",tablename);
    return 4;
  }


  /*checks if the specified column exists*/
  pos = name_list_pos(columnname,aTable->columns);
  if (pos < 0)
  {
    error("madxn.c: pro_ptc_select","Can not find column named <<%s>> in table <<%s>>.",
          columnname,aTable->name);
    return 7;
  }

  pos = name_list_pos("name",aTable->columns);
  if (pos < 0)
  {
    warning("madxn.c: pro_ptc_selectaTable->name: There is no column named <<name>> in table <<%s>>.",aTable->name);
    return 8;
  }

  /*so none of the columns is filled */
  aTable->org_cols = aTable->num_cols;


  *tabnameIA = new_int_array(1+strlen(tablename));
  conv_char(tablename,*tabnameIA);

  return 0;
}
/********************************************************************************/
int pro_ptc_moments(struct in_cmd* cmd)
{

  int no = (int)command_par_value("no", cmd->clone);

  w_ptc_moments(&no);

  return 1;
}


/********************************************************************************/
int pro_ptc_select_moment(struct in_cmd* cmd)
{
  /*adds a moment or more moments to the list
    if parametric switch is present they will be stored also as Taylor Series*/

  int pos, tablepos;
  int i, j;
  int mdefi[6];
  char* mdefin, *pchar;
  char  tablename[48];
  char  colname[8];
  int   clen = 0;
  struct int_array*              tabIA      = 0x0;
  struct int_array*              mdefIA      = 0x0;
  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            parametric = 0;
  int                            int_arr[100];


  tablepos = name_list_pos("table", c_parnames);
  if ( tablepos < 0)
  {
    printf("Weired: table parameter is not defined\n");
    return 1;
  }

  pchar  = c_parameters->parameters[tablepos]->string;
  if ( pchar == 0x0 )
  {
    strcpy(tablename,"moments");
  }
  else if ( pchar[0] == 0 )
  {
    strcpy(tablename,"moments");
  }
  else
  {
    strcpy(tablename,pchar);
  }

  tabIA = new_int_array(1+strlen(tablename));
  conv_char(tablename,tabIA);

  pos = name_list_pos("moment_s", c_parnames);
  if ( pos < 0)
  {
    printf("Weired: moments parameter is not defined\n");
    return 1;
  }

  if (c_parnames->inform[pos])
  {
    for (j = 0; j < c_parameters->parameters[pos]->m_string->curr; j++)
    {
      strcpy(colname,"mu000000");

      mdefin = c_parameters->parameters[pos]->m_string->p[j];

      /*printf("String no %d is %s\n", j, mdefin);*/

      clen = strlen(mdefin);

      /*the loop below decodes string monomial to integer monomial */
      for (i = 0; i< 6; i++)
      {
        if (clen > i)
        {
          mdefi[i] = mdefin[i] - '0';
          colname[2+i] = mdefin[i];
        }
        else
        {
          mdefi[i] = 0;
        }
      }


      mdefIA = new_int_array(9);/*size + "mu" + 6charnumbers*/
      conv_char(colname,mdefIA);

      w_ptc_addmoment(&(mdefi[0]),&(mdefi[1]),&(mdefi[2]),&(mdefi[3]),&(mdefi[4]),&(mdefi[5]),
                      tabIA->i, mdefIA->i, &parametric);

      delete_int_array(mdefIA);

    }
  }


  pos = name_list_pos("moment", c_parnames);
  /*i is dummy... we know there is no strings or doubles */
  comm_para_("moment", &pos, &i, &i, int_arr, 0x0, 0x0, 0x0);

  if (pos > 6) pos = 6;
/*if there is something and it is not only one zero */
  if ( (pos >= 0)       && !((pos == 1) && (int_arr[0] == 0))  )
  {

    for (i=0;i<pos;i++)
    {
      if (int_arr[i] < 0) break;

      mdefi[i] = int_arr[i];
    }

    for (i=pos; i<6;i++)
    {
      mdefi[i] = 0;
    }

    tablepos = name_list_pos("column", c_parnames);
    if ( tablepos < 0)
    {
      printf("Weired: column parameter is not defined\n");
      return 1;
    }

    tablename[0] = 0;
    pchar  = c_parameters->parameters[tablepos]->string;
    if ( pchar != 0x0 )
    {
      if ( pchar[0] != 0 )
      {
        strcpy(tablename,pchar);
      }

    }

    if (tablename[0] == 0)
    {
      sprintf(tablename,"mu_%d_%d_%d_%d_%d_%d",
              mdefi[0],mdefi[1],mdefi[2],mdefi[3],mdefi[4],mdefi[5]);
      printf("pro_ptc_select_moment: Column name not provied, generated one is %s",tablename);
    }

    mdefIA = new_int_array(1+strlen(tablename));

    conv_char(tablename,mdefIA);
    w_ptc_addmoment(&(mdefi[0]),&(mdefi[1]),&(mdefi[2]),&(mdefi[3]),&(mdefi[4]),&(mdefi[5]),
                    tabIA->i, mdefIA->i, &parametric);

    delete_int_array(mdefIA);
  }



  delete_int_array(tabIA);

  return 0;

}
/********************************************************************************/

int makemomentstables()
{
  static const int maxtables = 100;
  char*             tables[100];           /*tables[maxtables];*/
  struct name_list* cols[100];  /*cols[maxtables][maxcols];*/
  struct table*     t;
  char              tabname[20];
  char              colname[17];
  int               nmom;
  int i,j,k;


  memset(tables,0x0,maxtables*sizeof(char*));


  nmom = w_ptc_getnmoments();
  for (i = 1; i <= nmom; i++)
  {
    w_ptc_getmomentstabcol(&i, tabname, colname);
    /*printf(" mom %d: %s %s\n",i, tabname, colname);*/

    for(j=0; tables[j] != 0x0 ;j++)
    {
      if ((strcmp(tables[j],tabname) == 0)) break;
    }
    /*printf(" index of this table is %d \n",j);*/

    if (tables[j] == 0x0)
    {
      tables[j] = (char*)mycalloc("makemomentstables",strlen(tabname) + 1, sizeof(char));
      strcpy(tables[j],tabname);
      cols[j] = new_name_list("columns", 15);
      add_to_name_list(permbuff("name"),3,cols[j]);
      add_to_name_list(permbuff("s"),2,cols[j]);
    }

    k = add_to_name_list(permbuff(colname),2,cols[j]);

  }

  if (moments_tables)
  {
    myfree("",moments_tables->tables);
    delete_name_list(moments_tables->names);
    myfree("",moments_tables);
  }

  moments_tables = new_table_list(10);

  for(j=0; tables[j] != 0x0 ;j++)
  {
    /*printf("Making table %s\n",tables[j]);*/

    t = new_table(tables[j], "usermoments", current_sequ->n_nodes, cols[j]);
    t->org_cols = cols[j]->curr;
    /*print_table(t);*/
    add_to_table_list(t, table_register);
    add_to_table_list(t, moments_tables);
  }


/*  make_table("moments", "twiss", twiss_table_cols,
    twiss_table_types, current_sequ->n_nodes); */
  return 0;
}
/********************************************************************************/
void augmentcountmomtabs(double* s)
{
  int i;
  struct table* t;

  if (moments_tables == 0x0)
  {
    warning("augmentcountmomtabs","moments_tables is NULL\n");
    return;
  }

  for ( i = 0; i <  moments_tables->curr; i++)
  {
    t = moments_tables->tables[i];
    t->s_cols[0][t->curr] = tmpbuff(current_node->name);
    t->d_cols[1][t->curr] = *s;
    if (t->num_cols > t->org_cols)  add_vars_to_table(t);
    if (++t->curr == t->max) grow_table(t);
  }
}

void pro_ptc_script(struct in_cmd* cmd)
{/*
   processes ptc_script command
   it directs ptc_twiss to store given QUANTITY in a named TABLE's COLUMN
   Then, it these values are accessible for other MAD-X modules for calculations.
   The most important one is the matching module.
 */

  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;
  char*                          scriptname   = 0x0;
  struct int_array*              scriptnameIA = 0x0;/*string passing to fortran is tricky*/

  /*extracts table specified by the user*/
  pos   = name_list_pos("file", c_parnames);
  if (pos < 0)
  {
    printf("madxn.c: pro_ptc_script: file parameter does not exist.\n");
    return;
  }

  scriptname  = c_parameters->parameters[pos]->string;
  if ( scriptname == 0x0 )
  {
    warning("madxn.c: pro_ptc_script: no script name: ", "ignored");
    return;
  }

  scriptnameIA = new_int_array(1+strlen(scriptname));
  conv_char(scriptname,scriptnameIA);

  w_ptc_script_(scriptnameIA->i);/*calls the fortran*/

  delete_int_array(scriptnameIA);

}

/********************************************************************************/

void pro_ptc_open_gino(struct in_cmd* cmd)
{
/*
  processes ptc_open_gino command
*/

  struct command_parameter_list* c_parameters= cmd->clone->par;
  struct name_list*              c_parnames  = cmd->clone->par_names;
  int                            pos         = 0;
  char*                          scriptname   = 0x0;
  struct int_array*              scriptnameIA = 0x0;/*string passing to fortran is tricky*/

  pos   = name_list_pos("command", c_parnames);
  scriptname  = c_parameters->parameters[pos]->string;
  if ( scriptname == 0x0 )
  {
    warning("madxn.c: pro_ptc_open_gino: no script name: ", "ignored");
    return;
  }

  scriptnameIA = new_int_array(1+strlen(scriptname));
  conv_char(scriptname,scriptnameIA);

  w_ptc_open_gino_(scriptnameIA->i);/*calls the fortran*/

  delete_int_array(scriptnameIA);

}

/********************************************************************************/
void pro_ptc_track(struct in_cmd* cmd)
{
  int k=0, pos, one = 1;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
/*  char rout_name[] = "ptc_track"; */
  int npart = stored_track_start->curr;
  struct table* t;
/*  int turns = command_par_value("turns", cmd->clone); */

  track_is_on = 1;
  puts("enter PTC_TRACK module");
  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("sequence not active,", "TRACK ignored");
    return;
  }
  if (attach_beam(current_sequ) == 0)
    fatal_error("TRACK - sequence without beam:", current_sequ->name);
  if ((k = get_value(current_command->name,"onepass")) != 0)
    fprintf(prt_file, "one pass is on\n");
  /*
    if ((k = get_value(current_command->name,"damp")) != 0)
    fprintf(prt_file, "damp is on\n");
    set_option("damp", &k);
    if ((k = get_value(current_command->name,"quantum")) != 0)
    fprintf(prt_file, "quantum is on\n");
    set_option("quantum", &k);
  */
  set_option("onepass", &k);
  if ((k = get_value(current_command->name,"aperture")) != 0)
    fprintf(prt_file, "aperture tracking is on\n");
  set_option("aperture", &k);
  k = get_value(current_command->name,"dump");
  set_option("track_dump", &k);
  k = get_value(current_command->name,"onetable");
  set_option("onetable", &k);
  track_deltap=get_value(current_command->name,"deltap");
  set_variable("track_deltap", &track_deltap);
  if(track_deltap != 0) fprintf(prt_file, v_format("track_deltap: %F\n"),
                                track_deltap);
  pos = name_list_pos("file", nl);
  if (nl->inform[pos]) set_option("track_dump", &one);
  if ((track_filename = pl->parameters[pos]->string) == NULL)
  {
    if (pl->parameters[pos]->call_def != NULL)
      track_filename = pl->parameters[pos]->call_def->string;
    else track_filename = permbuff("dummy");
  }
  track_filename = permbuff(track_filename);
  track_fileext = NULL;
  pos = name_list_pos("extension", nl);
  if ((track_fileext = pl->parameters[pos]->string) == NULL)
  {
    if (pl->parameters[pos]->call_def != NULL)
      track_fileext = pl->parameters[pos]->call_def->string;
    if (track_fileext == NULL)  track_fileext = permbuff("\0");
  }
  track_fileext = permbuff(track_fileext);

  if (npart == 0)
  {
    warning("track_run: no START command seen yet", "ignored");
    return;
  }
  track_tables_create(cmd);
  printf("obs_points ptc_track: %d \n",curr_obs_points);
  w_ptc_track_(&curr_obs_points);
  t =
    table_register->tables[name_list_pos("tracksumm", table_register->names)];
  if (get_option("info"))  print_table(t);
  if (get_option("track_dump")) track_tables_dump();
  fprintf(prt_file, "\n*****  end of ptc_run  *****\n");
}

void track_ripple(struct in_cmd* cmd)
{
  if (track_is_on == 0)
  {
    warning("track_ripple: no TRACK command seen yet", "ignored");
    return;
  }
  puts("entered track_ripple routine");
}

void track_start(struct command* comm)
{
  char name[FNAME_L];
  if (track_is_on == 0)
  {
    warning("track_start: no TRACK command seen yet", "ignored");
    return;
  }
  track_start_cnt++;
  strcpy(name, "start.");
  sprintf(c_dum->c, "%d", track_start_cnt);
  strcat(name, c_dum->c);
  add_to_command_list(name,comm,stored_track_start,1);
}

void track_tables_create(struct in_cmd* cmd)
{
  int i, j;
  char tab_name[NAME_L];
  struct table* t;
  int t_size;
  int turns = command_par_value("turns", cmd->clone);
  int ffile = command_par_value("ffile", cmd->clone);
  if (ffile <= 0) ffile = 1;
  t_size = turns / ffile + 10;
  t = make_table("tracksumm", "tracksumm", tracksumm_table_cols,
                 tracksumm_table_types, 2*stored_track_start->curr);
  add_to_table_list(t, table_register);
  if (get_option("recloss"))
  {
    t = make_table("trackloss", "trackloss", trackloss_table_cols,
                   trackloss_table_types, stored_track_start->curr*t_size);
    add_to_table_list(t, table_register);
  }
  if (get_option("onetable"))
  {
    t = make_table("trackone", "trackone", trackone_table_cols,
                   trackone_table_types, stored_track_start->curr*t_size);
    add_to_table_list(t, table_register);
  }
  else
  {
    for (i = 0; i < curr_obs_points; i++)
    {
      for (j = 0; j < stored_track_start->curr; j++) /* open tables */
      {
        sprintf(tab_name, "track.obs%04d.p%04d", i+1, j+1);
        t = make_table(tab_name, "trackobs", track_table_cols,
                       track_table_types, t_size);
        add_to_table_list(t, table_register);
      }
    }
  }
}

void track_tables_dump()
{
  int j;
  for (j = 0; j < table_register->names->curr; j++)
  {
    if (strstr(table_register->names->names[j], "track.obs")
        || strcmp(table_register->names->names[j], "trackone") == 0)
    {
      strcpy(l_wrk->c, track_filename);
      strcat(l_wrk->c, &table_register->names->names[j][5]);
      strcat(l_wrk->c, track_fileext);
      out_table("track", table_register->tables[j], l_wrk->c);
    }
  }
}

void track_track(struct in_cmd* cmd)
{
  int k=0, pos, one = 1;
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;

  if (current_sequ == NULL || current_sequ->ex_start == NULL)
  {
    warning("sequence not active,", "TRACK ignored");
    return;
  }
  if (attach_beam(current_sequ) == 0)
    fatal_error("TRACK - sequence without beam:", current_sequ->name);
  if (track_is_on)
  {
    warning("already inside TRACK command group,", "ignored");
    return;
  }
  track_is_on = 1;
  puts("enter TRACK module");
  if ((k = get_value(current_command->name,"onepass")) != 0)
    fprintf(prt_file, "one pass is on\n");
  set_option("onepass", &k);

  if ((k = get_value(current_command->name,"update")) != 0)
    fprintf(prt_file, "update is on\n");
  set_option("update", &k);


  if ((k = get_value(current_command->name,"damp")) != 0)
    fprintf(prt_file, "damp is on\n");
  set_option("damp", &k);
  if ((k = get_value(current_command->name,"quantum")) != 0)
    fprintf(prt_file, "quantum is on\n");
  set_option("quantum", &k);


  if ((k = get_value(current_command->name,"aperture")) != 0)
    fprintf(prt_file, "aperture tracking is on\n");
  set_option("aperture", &k);
  if ((k = get_value(current_command->name,"recloss")) != 0)
    fprintf(prt_file, "losses recorded\n");
  set_option("recloss", &k);
  k = get_value(current_command->name,"dump");
  set_option("track_dump", &k);
  k = get_value(current_command->name,"onetable");
  set_option("onetable", &k);
  track_deltap=get_value(current_command->name,"deltap");
  set_variable("track_deltap", &track_deltap);
  if(track_deltap != 0) fprintf(prt_file, v_format("track_deltap: %F\n"),
                                track_deltap);
  curr_obs_points = 1;  /* default: always observe at machine end */
  pos = name_list_pos("file", nl);
  if (nl->inform[pos]) set_option("track_dump", &one);
  if ((track_filename = pl->parameters[pos]->string) == NULL)
  {
    if (pl->parameters[pos]->call_def != NULL)
      track_filename = pl->parameters[pos]->call_def->string;
    else track_filename = permbuff("dummy");
  }
  track_filename = permbuff(track_filename);
  track_fileext = NULL;
  pos = name_list_pos("extension", nl);
  if ((track_fileext = pl->parameters[pos]->string) == NULL)
  {
    if (pl->parameters[pos]->call_def != NULL)
      track_fileext = pl->parameters[pos]->call_def->string;
    if (track_fileext == NULL)  track_fileext = permbuff("\0");
  }
  track_fileext = permbuff(track_fileext);
}

int twiss_input(struct command* tw)
  /* returns -1 if an invalid beta0 given,
     returns -1 if only betx or bety given,
     returns 1 if betx and bety are given,
     or a valid beta0 (which is then loaded), else 0 */
{
  struct name_list* nl = tw->par_names;
  struct command_parameter_list* pl = tw->par;
  struct command* beta;
  int i = -1, ret = 0, pos, sb = 0;
  char* name;
  double val;
  pos = name_list_pos("beta0", nl);
  if (nl->inform[pos] && (name = pl->parameters[pos]->string) != NULL)
  {
    if ((pos = name_list_pos(name, beta0_list->list)) > -1)
    {
      ret = 1;
      beta = beta0_list->commands[pos];
      do
      {
        i++;
        if (nl->inform[name_list_pos(nl->names[i], nl)] == 0) /* not read */
        {
          if (beta->par->parameters[i]->expr != NULL)
            val = expression_value(beta->par->parameters[i]->expr, 2);
          else val = beta->par->parameters[i]->double_value;
          pl->parameters[i]->double_value = val;
          nl->inform[name_list_pos(nl->names[i], nl)] = 1;
        }
      }
      while (strcmp(nl->names[i], "energy") != 0);
    }
    else ret = -1;
  }
  if (ret) return ret;
  /* if no beta0 given, betx and bety together set inval */
  if (nl->inform[name_list_pos("betx", nl)]) sb++;
  if (nl->inform[name_list_pos("bety", nl)]) sb++;
  if (sb)
  {
    if (sb < 2)  return -2;
    else         return 1;
  }
  else return 0;
}

void update_node_constraints(struct node* c_node, struct constraint_list* cl)
{
  int i, j, k;
  k = 1; set_option("match_local", &k); /* flag */
  if (c_node->cl == NULL) c_node->cl = new_constraint_list(cl->curr);
  for (j = 0; j < cl->curr; j++)
  {
    k = -1;
    for (i = 0; i < c_node->cl->curr; i++)
    {
      if (strcmp(cl->constraints[j]->name,
                 c_node->cl->constraints[i]->name) == 0) k = i;
    }
    if (k < 0)
    {
      if (c_node->cl->curr == c_node->cl->max)
        grow_constraint_list(c_node->cl);
      c_node->cl->constraints[c_node->cl->curr++] = cl->constraints[j];
      total_const++;
    }
    else c_node->cl->constraints[k] = cl->constraints[j];
  }
}

void update_sequ_constraints(struct sequence* sequ, struct constraint_list* cl)
{
  int i, j, k;
  if (sequ->cl == NULL) sequ->cl = new_constraint_list(10);
  for (j = 0; j < cl->curr; j++)
  {
    k = -1;
    for (i = 0; i < sequ->cl->curr; i++)
    {
      if (strcmp(cl->constraints[j]->name,
                 sequ->cl->constraints[i]->name) == 0) k = i;
    }
    if (k < 0)
    {
      if (sequ->cl->curr == sequ->cl->max)
        grow_constraint_list(sequ->cl);
      sequ->cl->constraints[sequ->cl->curr++] = cl->constraints[j];
      total_const++;
    }
    else sequ->cl->constraints[k] = cl->constraints[j];
  }
}

void use_sequ(struct in_cmd* cmd)
{
  char rout_name[] = "use_sequ";
  struct name_list* nl = cmd->clone->par_names;
  struct command_parameter_list* pl = cmd->clone->par;
  int pos, lp;
  char* name;
  struct command* keep_beam = current_beam;
  if (sequ_is_on)
    fatal_error("no endsequence yet for sequence:", current_sequ->name);
  pos = name_list_pos("period", nl);
  if (nl->inform[pos] == 0) pos = name_list_pos("sequence", nl);
  if (nl->inform[pos])  /* parameter has been read */
  {
    if (current_range != NULL)
    {
      myfree(rout_name, current_range); current_range = NULL;
    }
    name = pl->parameters[pos]->string;
    if ((pos = name_list_pos(name, line_list->list)) > -1
        && line_list->macros[pos]->dead == 0)
      make_sequ_from_line(name); /* only if not disabled */
    if ((lp = name_list_pos(name, sequences->list)) > -1)
    {
      current_sequ = sequences->sequs[lp];
      if (attach_beam(current_sequ) == 0)
        fatal_error("USE - sequence without beam:", current_sequ->name);
      current_sequ->beam = current_beam;
      pos = name_list_pos("range", nl);
      if (nl->inform[pos])  /* parameter has been read */
        current_range = tmpbuff(pl->parameters[pos]->string);
      expand_curr_sequ(0);
    }
    else warning("unknown sequence skipped:", name);
  }
  current_beam = keep_beam;
}

void vector_to_table(char* table, char* col, int* nval, double* vals)
  /* puts nval values of array vals at the current line into columns
     starting with column whose name is in "col";
     The table count is increased separately with "augment_count" */
{
  int j, pos, c_pos, last = 0;
  struct table* t;

  mycpy(c_dum->c, table);
  if ((pos = name_list_pos(c_dum->c, table_register->names)) > -1)
    t = table_register->tables[pos];
  else return;
  mycpy(c_dum->c, col);
  if ((c_pos = name_list_pos(c_dum->c, t->columns)) > -1)
    last = mymin(c_pos + *nval, t->num_cols);
  for (j = c_pos; j < last; j++)
    if (t->columns->inform[j] < 3) t->d_cols[j][t->curr] = vals[j-c_pos];
}

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/


int getnumberoftracks()
{
/*returns number of input tracks */
  if (stored_track_start == 0x0)
  {
    return 0;
  }

  return stored_track_start->curr;

}

int  getcurrentcmdname(char* string)
{
  if (current_command == 0x0)
  {
    return 0;
  }

  strcpy(string, current_command->name);
  return strlen(current_command->name);

}

const char* getcurrentelementname()
{
/*returns number of input tracks */

  if (current_node == 0x0)
  {
    return 0x0;
  }

  return current_node->name;

}

/***************************************************************************/

int copytrackstoarray()
{
  /*copies track positions from commands to array
    returns number of copied tracks, value <= 0 in case of error
  */
  /**/
  int ntracks = 0;/*number of tracks : returned value */
  int n = 0; /*interator over tracks*/
  struct command* comm;
  if (trackstrarpositions)
  {
    deletetrackstrarpositions();
  }

  ntracks = getnumberoftracks();
  if (ntracks <= 0)
  {
    printf("ERROR: copytrackstoarray: number of tracks is 0! Nothing to copy!");
    return 0;
  }
  trackstrarpositions =  (double**)mymalloc("copytrackstoarray",ntracks*sizeof(double*));

  for (n = 0; n < ntracks; n++)
  {
    trackstrarpositions[n] = (double*)mymalloc("copytrackstoarray",6*sizeof(double));

    comm = stored_track_start->commands[n];
    trackstrarpositions[n][0] = command_par_value("x",  comm);
    trackstrarpositions[n][1] = command_par_value("px", comm);
    trackstrarpositions[n][2] = command_par_value("y",  comm);
    trackstrarpositions[n][3] = command_par_value("py", comm);
    trackstrarpositions[n][4] = command_par_value("t",  comm);
    trackstrarpositions[n][5] = command_par_value("pt", comm);

  }
  return ntracks;

}
/***************************************************************************/


int gettrack(int* nt, double* x,double* px,double* y,double* py,double* t,double* pt)
{
  /* returns the parameters of track n;
     0 = none, else count */
  int n = *nt - 1;

  if ( trackstrarpositions == 0x0 )
  {
    copytrackstoarray();
  }
  if ( (n<0) || (n >= stored_track_start->curr) )
  {
    printf("gettrack: track number %d out of range",n);
    return 1;
  }


  *x      = trackstrarpositions[n][0];
  *px     = trackstrarpositions[n][1];
  *y      = trackstrarpositions[n][2];
  *py     = trackstrarpositions[n][3];
  *t      = trackstrarpositions[n][4];
  *pt     = trackstrarpositions[n][5];
  return 0;
}

/***************************************************************************/

void deletetrackstrarpositions()
{
  /* deletes the array with track positions */
  int i;
  for ( i = 0; i < stored_track_start->curr; i++)
  {
    myfree("deletetrackstrarpositions",trackstrarpositions[i]);
  }
  myfree("deletetrackstrarpositions",trackstrarpositions);

  trackstrarpositions = 0x0;
}
/***************************************************************************/