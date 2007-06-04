/* Geomap - A DBus api and wrapper for getting geography pictures
 * Copyright (C) 2006 Garmin
 * 
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2 as published by the Free Software Foundation; 
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
#include <gtk/gtk.h>
#include <geoclue/find.h>

               
void results(char** name, char** description, GArray* latitude, GArray* longitude , char** address, char** city , char** state, char** zip, void* userdata )
{
 printf("found results\n");   
 int i = 0;
 for( i = 0; name[i] != NULL; i++)
 {
    printf("%d: %s \n%s\n%f %f\n", i, name[i], address[i], g_array_index(latitude,double,i) , g_array_index(longitude,double,i));
   // free(name[i]);
   // free(address[i]);
    
 }
 //free(name);
 //free(address);
 //g_array_free(latitude, TRUE);
 //g_array_free(longitude, TRUE); 
}



int main(int argc, char **argv) {
  
    g_type_init();
    GMainLoop*  loop = g_main_loop_new(NULL,FALSE);

    
    if(argv[1] == NULL)
    {
        printf("Please enter search term as arguement (geoclue-find-example \"wifi\"\n"); 
        return(1);  
    }
        
    
    if( geoclue_find_init())
    {   
        g_print("Error Opening Geomap\n");      
    }    
    
    char** categories;
    char** subcategories;
    GEOCLUE_FIND_RETURNCODE retncode;
    

    if(geoclue_find_top_level_categories(&categories) == GEOCLUE_FIND_SUCCESS)
    {
          
        int i,j;
        for(i=0; categories[i] != NULL; i++)
        {
            if(geoclue_find_subcategories(categories[i], &subcategories) == GEOCLUE_FIND_SUCCESS)
            {
                printf("New Category %s\n", categories[i]);
                
                for(j=0; subcategories[j] != NULL; j++)
                {
                    printf("\t%s\n", subcategories[j]);
                    free(subcategories[j]);
                }
                free(subcategories);
                
                
                free(categories[i]);   
            }
        }
        free(categories);
    }

    gint id;
    geoclue_find_find_near (38.0,-95.0, "Food", argv[1], results, NULL);  
    g_main_run(loop); 
    

    return(0);
}


