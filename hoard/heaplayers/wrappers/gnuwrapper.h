extern "C" {
  void * xxmalloc (size_t);
  void   xxfree (void *);
  void * xxrealloc (void *ptr, size_t sz);
  void *get_page_list(unsigned int *pgcount);	
  void migrate_pages(int node);	
  size_t xx_get_largemmap_range (void *start, void *end);	
  void* xx_reserve(size_t sz);
}
