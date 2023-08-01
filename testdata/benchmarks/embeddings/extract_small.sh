#!/bin/bash

# Simply extract the three smallest benchmark embeddings.
tar --wildcards -zxvf benchmark_models.tar.gz ./littlefs-reg2mem.*  
tar --wildcards -zxvf benchmark_models.tar.gz ./pidgin-reg2mem.*  
tar --wildcards -zxvf benchmark_models.tar.gz ./zlib-reg2mem.* 
