import os
import shutil
import random
import os.path

#src_dir = 'C:\\YS\\LyricExtractor_release(2020_05_06)\\MV_karaoke_20200416'
src_dir = 'E:\\4. TV 노래방(태국향2)\\0. 필수파일들\\1. 음악파일\\0. 원본 파일\\40001-80000(MV)\\1. 분할'
target_dir = 'C:\\YS\\LyricExtractor_release(2020_05_06)\\MV_karaoke_20200617'
copyFileCount = 100

src_files = (os.listdir(src_dir))
src_files_mp4 = [file for file in src_files if file.endswith(".mp4")]

print('from ' + src_dir )
print('to ' + target_dir )

#print(src_files_mp4)

def valid_path(dir_path, filename):
       full_path = os.path.join(dir_path, filename)
       return os.path.isfile(full_path)  

files = [os.path.join(src_dir, f) for f in src_files_mp4 if valid_path(src_dir, f)]
choices = random.sample(files, copyFileCount)
print(choices)
for files in choices:
       print('copy... : ' + files)
       shutil.copy(files, target_dir)
print ('Finished!')