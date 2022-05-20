#define FUSE_USE_VERSION 30

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <curl/curl.h>
// ... //
//	DONT PLEASE FORGET TO FREE STRING POINTER!!!!

//for weather preparation 

struct string {
	  char *ptr;
	    size_t len;
};

void init_string(struct string *s) {
	  s->len = 0;
	    s->ptr = malloc(s->len+1);
	      if (s->ptr == NULL) {
		          fprintf(stderr, "malloc() failed\n");
			      exit(EXIT_FAILURE);
			        }
	        s->ptr[0] = '\0';
}


struct string s;


size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s)
{
	  size_t new_len = s->len + size*nmemb;
	    s->ptr = realloc(s->ptr, new_len+1);
	      if (s->ptr == NULL) {
		          fprintf(stderr, "realloc() failed\n");
			      exit(EXIT_FAILURE);
			        }
	        memcpy(s->ptr+s->len, ptr, size*nmemb);
		  s->ptr[new_len] = '\0';
		    s->len = new_len;

		      return size*nmemb;
}

void get_weather_string(const char * city)
{
	  CURL *curl;
	    CURLcode res;
		char a[32]={"http://wttr.in/"};
		char b[32]={"?0"};
		strcat(a,city);
		strcat(a,b);

		  init_string(&s);
	      curl = curl_easy_init();
	        if(curl) {
			  

	       	    curl_easy_setopt(curl, CURLOPT_URL, a);
		        curl_easy_setopt(curl, CURLOPT_USERAGENT, "curl/7.47.1");
		        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
		        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
			    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);
		        res = curl_easy_perform(curl);

					//printf("somethinthere%s\nandthere", s.ptr);
					   //     free(s.ptr);

						/* always cleanup */
			    curl_easy_cleanup(curl);
	      }
		 // printf("\n!from get_weather_string function: %s\n",s.ptr);
		  
}

char dir_list[ 256 ][ 256 ];
int curr_dir_idx = -1;

char files_list[ 256 ][ 256 ];
int curr_file_idx = -1;

char files_content[ 256 ][ 1024 ];
int curr_file_content_idx = -1;

time_t dir_time_access[256];//
time_t dir_time_mod[256];


time_t file_time_access[ 256];
//int curr_file_time_access=-1;

time_t file_time_mod[ 256];
//int curr_file_time_mod = -1;

//we need to implement one more string array for time_a and time_m  and implement correct wether representing into files 


void add_dir( const char *dir_name )
{
	curr_dir_idx++;
	strcpy( dir_list[ curr_dir_idx ], dir_name );
	time(&dir_time_access[curr_dir_idx]);
	time(&dir_time_mod[curr_dir_idx]);

}

int is_dir( const char *path )
{
	path++; 
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_dir_idx( const char *path )
{
	path++; 
	
	for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
		if ( strcmp( path, dir_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

void add_file( const char *filename )
{
	//zdes' nado realizovat' time_m and access 
	curr_file_idx++;
	strcpy( files_list[ curr_file_idx ], filename );
	time(&file_time_access[curr_file_idx]);
	time(&file_time_mod[curr_file_idx]);
		
	//file content == weather 
	curr_file_content_idx++;

	get_weather_string(filename);
	strcpy( files_content[ curr_file_content_idx ], s.ptr);
	free(s.ptr);
}

int is_file( const char *path )
{
	path++; // Eliminating "/" in the path
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return 1;
	
	return 0;
}

int get_file_index( const char *path )
{
	path++; 
	
	for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
		if ( strcmp( path, files_list[ curr_idx ] ) == 0 )
			return curr_idx;
	
	return -1;
}

void write_to_file( const char *path, const char *new_content )
{
	int file_idx = get_file_index( path );

	if ( file_idx == -1 ) // No such file
		return;

	time(&file_time_mod[file_idx]);
	strcpy( files_content[ file_idx ], new_content ); 
}



static int do_getattr( const char *path, struct stat *st )
{
	st->st_uid = getuid(); 
	st->st_gid = getgid(); //исправить
	 
	int cur_idx = -1;
	if ( strcmp( path, "/" ) == 0 || is_dir( path ) == 1 )
	{
		cur_idx=get_dir_idx(path);
		st->st_atime = dir_time_access[cur_idx]; 
		st->st_mtime = dir_time_mod[cur_idx] ;
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2; 
	}
	else if ( is_file( path ) == 1 )
	{
		cur_idx=get_file_index(path);
		st->st_atime = file_time_access[cur_idx]; 
		st->st_mtime = file_time_mod[cur_idx];
		st->st_mode = S_IFREG | 0444;
		st->st_nlink = 1;
		st->st_size = strlen(files_content[cur_idx]);
	}
	else
	{
		return -ENOENT;
	}
	
	return 0;
}

static int do_readdir( const char *path, void *buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi )
{
	filler( buffer, ".", NULL, 0 ); // Current Directory
	filler( buffer, "..", NULL, 0 ); // Parent Directory
	
	if ( strcmp( path, "/" ) == 0 ) 
	{
		for ( int curr_idx = 0; curr_idx <= curr_dir_idx; curr_idx++ )
			filler( buffer, dir_list[ curr_idx ], NULL, 0 );
	
		for ( int curr_idx = 0; curr_idx <= curr_file_idx; curr_idx++ )
			filler( buffer, files_list[ curr_idx ], NULL, 0 );
	}
	
	return 0;
}

static int do_read( const char *path, char *buffer, size_t size, off_t offset, struct fuse_file_info *fi )
{
	int file_idx = get_file_index( path );
	
	if ( file_idx == -1 )
		return -1;
	time_t now = time(NULL);
	if((now - file_time_mod[file_idx]) > 10800)
	{
        const char * cityname = path+1;
        get_weather_string(cityname);
		write_to_file(path,s.ptr);
		time(&file_time_mod[file_idx]);
	}
	time(&file_time_access[file_idx]);
	char *content = files_content[ file_idx ];
	
	memcpy( buffer, content + offset, size );
		
	return strlen( content ) - offset;
}

static int do_mkdir( const char *path, mode_t mode )
{
	path++;
	add_dir( path );
	
	return 0;
}

static int do_mknod( const char *path, mode_t mode, dev_t rdev )
{
	path++;
	add_file( path );
	
	return 0;
}

static int do_write( const char *path, const char *buffer, size_t size, off_t offset, struct fuse_file_info *info )
{
	write_to_file( path, buffer );
	
	return size;
}

static struct fuse_operations operations = {
    .getattr	= do_getattr,
    .readdir	= do_readdir,
    .read		= do_read,
    .mkdir		= do_mkdir,
    .mknod		= do_mknod,
    .write		= do_write,
};

int main( int argc, char *argv[] )
{
	return fuse_main( argc, argv, &operations, NULL );
}
