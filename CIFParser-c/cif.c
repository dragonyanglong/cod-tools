/*---------------------------------------------------------------------------*\
**$Author$
**$Date$ 
**$Revision$
**$URL$
\*---------------------------------------------------------------------------*/

/* representation of the cif for the interpreter, assembler and
   code generator */

/* exports: */
#include <cif.h>

/* uses: */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <allocx.h>
#include <assert.h>
#include <cexceptions.h>
#include <cxprintf.h>
#include <stringx.h>

void *cif_subsystem = &cif_subsystem;

static int cif_debug = 0;

void cif_debug_on( void )
{
    cif_debug = 1;
}

int cif_debug_is_on( void )
{
    return cif_debug;
}

void cif_debug_off( void )
{
    cif_debug = 0;
}

#define DELTA_CAPACITY (100)

struct CIF {
    /* Fields 'length' and 'capacity' describe allocated parallel
       arrays tags and values. The 'capacity' field contains a number
       of array elements allocated for use in each array. The 'length'
       field contains a number of elements actually used. All elements
       after the 'length-1'-th element MAY contain NULL values. */
    size_t length;
    size_t capacity;
    ssize_t tags_length;
    char **tags;
    char ***values;
    cif_value_type_t *types;

    ssize_t loop_start; /* Index of the entry into the 'tags',
                           'values' and 'types' arrays that indicates
                           the beginning of the currently compiled
                           loop. */
    ssize_t loop_current; /* Index of the 'values' and 'types' arrays
                             where a new loop value will be pushed. */


    int loop_count;    /* Number of loops in the array 'loops' and 'loop_sizes'. */
    int *loop_lengths; /* Each element contains length of loop 'loops[i]'. */
    int **loops;       /* Each element contains an array, of length
                          'loop_sizes[i]', that lists all tags that
                          belong to this loop. */
};

CIF *new_cif( cexception_t *ex )
{
    CIF *cif = callocx( 1, sizeof(CIF), ex );
    cif->loop_start = -1;
    return cif;
}

void delete_cif( CIF *cif )
{
    ssize_t i, j;

    if( cif ) {
        for( i = 0; i < cif->capacity; i++ ) {
            if( cif->tags ) 
                freex( cif->tags[i] );
            if( cif->values && cif->values[i] ) {
                for( j = 0; cif->values[i][j] != NULL; j++ )
                    freex( cif->values[i][j] );
                freex( cif->values[i] );
            }
        }
        for( i = 0; i < cif->loop_count; i++ ) {
            freex( cif->loops[i] );
        }
        freex( cif->tags );
        freex( cif->values );
        freex( cif->types );
        freex( cif->loop_lengths );
        freex( cif->loops );
	freex( cif );
    }
}

void create_cif( CIF * volatile *cif, cexception_t *ex )
{
    assert( cif );
    assert( !(*cif) );

    *cif = new_cif( ex );
}

void dispose_cif( CIF * volatile *cif )
{
    assert( cif );
    if( *cif ) {
	delete_cif( *cif );
	*cif = NULL;
    }
}

void cif_dump( CIF * volatile cif )
{
    ssize_t i;

    for( i = 0; i < cif->length; i++ ) {
        switch( cif->types[i] ) {
        case CIF_NUMBER:
        case CIF_UQSTRING:
            printf( "%-32s %s\n", cif->tags[i], cif->values[i][0] );
            break;
        case CIF_SQSTRING:
            printf( "%-32s '%s'\n", cif->tags[i], cif->values[i][0] );
            break;
        case CIF_DQSTRING:
            printf( "%-32s \"%s\"\n", cif->tags[i], cif->values[i][0] );
            break;
        case CIF_TEXT:
            printf( "%s\n;%s\n;\n", cif->tags[i], cif->values[i][0] );
            break;
        default:
            fprintf( stderr, "unknown CIF value type %d from CIF parser!\n", cif->types[i] );
            printf( "%-32s '%s'\n", cif->tags[i], cif->values[i][0] );
            break;
        }
    }

    return;
}

void cif_insert_value( CIF * cif, char *tag,
                       char *value, cif_value_type_t vtype,
                       cexception_t *ex )
{
    cexception_t inner;
    ssize_t i;

    cexception_guard( inner ) {
        i = cif->length;
        if( cif->length + 1 > cif->capacity ) {
            cif->tags = reallocx( cif->tags,
                                  sizeof(cif->tags[0]) * (cif->capacity + DELTA_CAPACITY),
                                  &inner );
            cif->tags[i] = NULL;
            cif->values = reallocx( cif->values,
                                    sizeof(cif->values[0]) * (cif->capacity + DELTA_CAPACITY),
                                    &inner );
            cif->values[i] = NULL;
            cif->types = reallocx( cif->types,
                                   sizeof(cif->types[0]) * (cif->capacity + DELTA_CAPACITY),
                                   &inner );
            cif->values[i] = NULL;
            cif->capacity += DELTA_CAPACITY;
        }
        cif->length++;

        cif->values[i] = callocx( sizeof(cif->values[0][0]), 2, &inner );

        cif->tags[i] = tag;
        cif->values[i][0] = value;
        cif->types[i] = vtype;
    }
    cexception_catch {
        cexception_reraise( inner, ex );
    }
}

void cif_start_loop( CIF *cif )
{
    assert( cif );
    cif->loop_current = cif->loop_start = cif->length;
}

void cif_finish_loop( CIF *cif )
{
    assert( cif );
    cif->loop_current = cif->loop_start = -1;
}

void cif_push_loop_value( CIF * cif, char *value, cif_value_type_t vtype,
                          cexception_t *ex )
{
    cexception_t inner;
    ssize_t i, j;

    return;

    assert( cif->loop_start < cif->length );
    assert( cif->loop_current < cif->length );

    cexception_guard( inner ) {
        i = cif->loop_current;
        j = cif->loop_lengths[i];
        cif->values[i][j] = value;
        cif->types[i] = vtype;
        cif->loop_lengths[i] ++;
    }
    cexception_catch {
        cexception_reraise( inner, ex );
    }
}
