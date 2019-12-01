# Main settings includes
include	../../settings.mk

# Individual project settings (Optional)
BOOT		= new
APP		= 2
#SPI_SPEED	= 40
# 							Let op!
#							SPI_MODE = QIO (default) voor AI Cloud
#							SPI_MODE = DOUT voor merkloze
#SPI_MODE	= QIO			
#SPI_MODE	= DOUT
#SPI_SIZE_MAP	= 2
ESPPORT		= COM4
#ESPBAUD		= 256000

# Basic project settings
MODULES	= driver user
LIBS	= c gcc hal phy pp net80211 lwip wpa main crypto json upgrade

# Root includes
include	../../common_nonos.mk
