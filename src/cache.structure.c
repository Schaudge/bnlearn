#include "common.h"

#define NODE(i) CHAR(STRING_ELT(nodes, i))
#define AMAT(i,j) INTEGER(amat)[i + j * nrows]

#define BLANKET		 1
#define NEIGHBOUR	 2
#define PARENT		 3
#define CHILD		 4

SEXP cache_node_structure(int cur, SEXP nodes, SEXP amat, int nrows,
    int *status, SEXP debug) {

  int i = 0, j = 0;
  SEXP structure, names;
  SEXP mb, nbr, children, parents;
  int num_parents = 0, num_children = 0, num_neighbours = 0, num_blanket = 0;

  if (isTRUE(debug))
    Rprintf("* node %s.\n", NODE(cur));

  for (i = 0; i < nrows; i++) {

    if (AMAT(cur, i) == 1) {

      if (AMAT(i, cur) == 0) {

        /* if a[i,j] = 1 and a[j,i] = 0, then i -> j. */
        if (isTRUE(debug))
          Rprintf("  > found child %s.\n", NODE(i));

        status[i] = CHILD;

        /* check whether this child has any other parent. */
        for (j = 0; j < nrows; j++) {

          if ((AMAT(j,i) == 1) && (AMAT(i,j) == 0) && (j != cur)) {

            /* don't mark a neighbour as in the markov blanket. */
            if (status[j] <= 1) {

              status[j] = BLANKET;

              if (isTRUE(debug))
                Rprintf("  > found node %s in markov blanket.\n", NODE(j));

            }/*THEN*/

          }/*THEN*/

        }/*FOR*/   

      }/*THEN*/
      else {

        /* if a[i,j] = 1 and a[j,i] = 1, then i -- j. */
        if (isTRUE(debug))
          Rprintf("  > found neighbour %s.\n", NODE(i));

        status[i] = NEIGHBOUR;

      }/*ELSE*/

    }/*THEN*/
    else {

      if (AMAT(i, cur) == 1) {

        /* if a[i,j] = 0 and a[j,i] = 1, then i <- j. */
        if (isTRUE(debug))
          Rprintf("  > found parent %s.\n", NODE(i));

        status[i] = PARENT;

      }/*THEN*/

    }/*ELSE*/

  }/*FOR*/

  /* count how may nodes fall in each category. */
  for (i = 0; i < nrows; i++) {

    switch(status[i]) {

      case CHILD:
        /* a child is also a neighbour and belongs into the markov blanket. */
        num_children++;
        num_neighbours++;
        num_blanket++;
        break;
      case PARENT:
        /* the same goes for a parent. */
        num_parents++;
        num_neighbours++;
        num_blanket++;
        break;
      case NEIGHBOUR:
        /* it's not known if this is parent or a children, but it's certainly a neighbour. */
        num_neighbours++;
        num_blanket++;
        break;
      case BLANKET:
        num_blanket++;
        break;
      default:
        /* this node is not even in the markov blanket. */
        break;

    }/*SWITCH*/

  }/*FOR*/

  if (isTRUE(debug))
    Rprintf("  > node %s has %d parent(s), %d child(ren), %d neighbour(s) and %d nodes in the markov blanket.\n",
      NODE(cur), num_parents, num_children, num_neighbours, num_blanket);

  /* allocate and initialize the names of the elements. */
  PROTECT(names = allocVector(STRSXP, 4));
  SET_STRING_ELT(names, 0, mkChar("mb"));
  SET_STRING_ELT(names, 1, mkChar("nbr"));
  SET_STRING_ELT(names, 2, mkChar("parents"));
  SET_STRING_ELT(names, 3, mkChar("children"));

  /* allocate the list and set its attributes. */
  PROTECT(structure = allocVector(VECSXP, 4));
  setAttrib(structure, R_NamesSymbol, names);

  /* allocate and fill the "children" element of the list. */
  PROTECT(children = allocVector(STRSXP, num_children));
  for (i = 0, j = 0; (i < nrows) && (j < num_children); i++) {

    if (status[i] == CHILD)
      SET_STRING_ELT(children, j++, STRING_ELT(nodes, i));

  }/*FOR*/

  /* allocate and fill the "parents" element of the list. */
  PROTECT(parents = allocVector(STRSXP, num_parents));
  for (i = 0, j = 0; (i < nrows) && (j < num_parents); i++) {

    if (status[i] == PARENT)
      SET_STRING_ELT(parents, j++, STRING_ELT(nodes, i));

  }/*FOR*/

  /* allocate and fill the "nbr" element of the list. */
  PROTECT(nbr = allocVector(STRSXP, num_neighbours));
  for (i = 0, j = 0; (i < nrows) && (j < num_neighbours); i++) {

    if (status[i] >= NEIGHBOUR)
      SET_STRING_ELT(nbr, j++, STRING_ELT(nodes, i));

  }/*FOR*/

  /* allocate and fill the "mb" element of the list. */
  PROTECT(mb = allocVector(STRSXP, num_blanket));
  for (i = 0, j = 0; (i < nrows) && (j < num_blanket + num_neighbours); i++) {

    if (status[i] >= BLANKET)
      SET_STRING_ELT(mb, j++, STRING_ELT(nodes, i));

  }/*FOR*/

  /* attach the string vectors to the list. */
  SET_VECTOR_ELT(structure, 0, mb);
  SET_VECTOR_ELT(structure, 1, nbr);
  SET_VECTOR_ELT(structure, 2, parents);
  SET_VECTOR_ELT(structure, 3, children);

  UNPROTECT(6);

  return structure;

}/*CACHE_NODE_STRUCTURE*/

SEXP cache_structure(SEXP nodes, SEXP amat, SEXP debug) {

  int i = 0;
  int length_nodes = LENGTH(nodes);
  int *status;

  SEXP bn, temp;

  /* allocate the list and set its attributes.*/
  PROTECT(bn = allocVector(VECSXP, length_nodes));
  setAttrib(bn, R_NamesSymbol, nodes);

  /* allocate and intialize the status vector. */
  status = alloc1dcont(length_nodes);

  if (isTRUE(debug))
    Rprintf("* (re)building cached information about network structure.\n");

  /* populate the list with nodes' data. */
  for (i = 0; i < length_nodes; i++) {

    /* (re)initialize the status vector. */
    memset(status, '\0', sizeof(int) * length_nodes);

    temp = cache_node_structure(i, nodes, amat, length_nodes, status, debug);

    /* save the returned list. */
    SET_VECTOR_ELT(bn, i, temp);

  }/*FOR*/

  UNPROTECT(1);

  return bn;

}/*CACHE_STRUCTURE*/

SEXP cache_partial_structure(SEXP nodes, SEXP target, SEXP amat, SEXP debug) {

  int i = 0;
  int length_nodes = LENGTH(nodes);
  char *t = (char *)CHAR(STRING_ELT(target, 0));
  int *status;

  if (isTRUE(debug))
    Rprintf("* (re)building cached information about node %s.\n", t);

  /* allocate and initialize the status vector. */
  status = alloc1dcont(length_nodes);

  /* iterate fo find the node position in the array.  */
  for (i = 0; i < length_nodes; i++)
    if (!strcmp(t, CHAR(STRING_ELT(nodes, i))))
      break;

  /* return the corresponding part of the bn structure. */
  return cache_node_structure(i, nodes, amat, length_nodes, status, debug);

}/*CACHE_PARTIAL_STRUCTURE*/
