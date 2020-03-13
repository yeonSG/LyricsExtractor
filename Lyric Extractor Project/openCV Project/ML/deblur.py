import numpy as np
import random
import sys
import tensorflow as tf
import os

gpus = tf.config.experimental.list_physical_devices('GPU')
if gpus:
  # 텐서플로가 첫 번째 GPU에 1GB 메모리만 할당하도록 제한
  try:
    tf.config.experimental.set_virtual_device_configuration(
        gpus[0],
        [tf.config.experimental.VirtualDeviceConfiguration(memory_limit=1024)])
  except RuntimeError as e:
    # 프로그램 시작시에 가장 장치가 설정되어야만 합니다
    print(e)


def psnr_metric(y_true, y_pred):
    return tf.image.psnr(y_true, y_pred, max_val=1.0)

dependencies = {
    'psnr_metric':[psnr_metric]
}
    
def load_model_jpegDeblocking(path):
        dependencies = { 'psnr_metric':[psnr_metric] }
        loadModel = tf.keras.models.load_model(path+'\\'+'lyric_jpegdeblock_deblur_model.h5', custom_objects=dependencies)
        return loadModel
    
def saveImage(tensor, fileName, isTensor):
    if isTensor:
        np_float = tensor.numpy()
    else:
        np_float = tensor
        
    np_float.flags.writeable = True
    
    np_float[np_float < 0] = 0 # 0~1 사이 값만 남도록 보정
    np_float[np_float > 1] = 1
    
    np_int = (np_float*255).round().astype(np.uint8)
    from PIL import Image
    im = Image.fromarray(np_int)
    im.save(fileName+".png")

    
def main(filePath) :
    print("---Python Start---")
    print("image debluring"+filePath)    
    imgPath = os.path.split(filePath)

#이미지 로드
    img =  tf.io.read_file(filePath)
    img = tf.image.decode_jpeg(img, channels=3)
    img = tf.image.convert_image_dtype(img, tf.float32)

#모델 로드
    model_deblock = load_model_jpegDeblocking(imgPath[0])

# 디블로킹 이미지 생성 (int)
    img_resize = model_deblock.predict(np.expand_dims(img, axis=0))

    savePath = imgPath[0]+"\\deBlur_after"
    saveImage(np.squeeze(img_resize, axis=0), savePath, False)

# 메모리 할당 해제
    del model_deblock
    print("---Python End---")

if __name__ == "__main__" :
    main(sys.argv[1])