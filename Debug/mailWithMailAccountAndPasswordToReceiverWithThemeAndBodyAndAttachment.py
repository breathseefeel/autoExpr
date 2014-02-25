#!/usr/bin/pythonw
# -*- coding: utf-8 -*-
#导入smtplib和MIMEText
import email
import mimetypes
from email.MIMEMultipart import MIMEMultipart
from email.MIMEText import MIMEText
from email.MIMEImage import MIMEImage
import smtplib
import logging
import sys
import os
import time

reload(sys)
sys.setdefaultencoding('utf8')

mailDict = {} #邮件配置信息

###################
#日志辅助类
#################
class Logger:
    LOG_RELEASE= "releae"
    LOG_RELEASE_FILE = "./log/pyMail.log"

    def __init__(self, log_type):
        self._logger = logging.getLogger(log_type)
        if log_type == Logger.LOG_RELEASE:
            self._logFile = Logger.LOG_RELEASE_FILE

        handler = logging.FileHandler(self._logFile)
        if log_type == Logger.LOG_RELEASE:
            formatter = logging.Formatter('%(asctime)s ********* %(message)s')
        else:
            formatter = logging.Formatter('%(message)s')
        handler.setFormatter(formatter)
        self._logger.addHandler(handler)
        self._logger.setLevel(logging.INFO)

    def log(self, msg):
        if self._logger is not None:
            self._logger.info(msg)

MyLogger = Logger(Logger.LOG_RELEASE) #Logger

def initMailConf():#初始化邮件配置信息
    global mailDicti
    accountInfo = sys.argv[1].split('@')
    mailDict['server'] = "smtp."+accountInfo[1]
    mailDict['user'] = accountInfo[0]
    mailDict['password'] = sys.argv[2]
    mailDict["from"] = sys.argv[1]
    mailDict["cc"] = sys.argv[1]
#    mailDict["to"] = argv[3]
#    mailDict['server'] = "smtp.gmail.com"
#    mailDict['user'] = "swan1st"
#    mailDict['password'] = "5VrYs_4t"
#    mailDict["from"] = "swan1st@gmail.com"
#    mailDict["cc"] = "iamwansong@qq.com"
#    mailDict["to"] = "wansong_smart@163.com"
    if len(sys.argv) >= 5:
        if os.path.exists(sys.argv[4]):
            f = open(sys.argv[4])
            mailDict["subject"]  = f.read()
            f.close()
        else:
            mailDict["subject"] = sys.argv[4]

    if len(sys.argv) >= 6:
        if os.path.exists(sys.argv[5]):
            f = open(sys.argv[5])
            txt = f.read().replace('\n','<br/>')
            mailDict["text"] = txt
            mailDict["html"] = "%s" %txt #"<font color = red ><b>attention!%s</b></font>" %txt
            f.close()
        else:
            txt = sys.argv[5].replace('\n', '<br/>')
            mailDict["text"] = txt
            mailDict["html"] = "%s" %txt

def sendMail(paramMap):#发送邮件
    smtp = smtplib.SMTP()
    msgRoot = MIMEMultipart('related')
    msgAlternative = MIMEMultipart('alternative')
    if paramMap.has_key("server") and paramMap.has_key("user") and paramMap.has_key("password"):
        try:
            smtp.set_debuglevel(1)
            smtp.connect(paramMap["server"])
            smtp.login(paramMap["user"], paramMap["password"])
        except Exception, e:
            MyLogger.log("smtp login exception!")
            return False
    else:
        MyLogger.log("Parameters incomplete!")
        return False

    if paramMap.has_key("subject") == False or  paramMap.has_key("from")== False or paramMap.has_key("to") == False:
        MyLogger.log("Parameters incomplete!")
        return False
    msgRoot['subject'] = paramMap["subject"]
    msgRoot['from'] = paramMap["from"]
    if paramMap.has_key("cc"):
        msgRoot['cc'] = paramMap["cc"]
    msgRoot['to'] = paramMap["to"]
    msgRoot.preamble = 'This is a multi-part message in MIME format.'
    msgRoot.attach(msgAlternative)
    TempAddTo = paramMap["to"]
    if paramMap.has_key("text"):
        msgText = MIMEText(paramMap["text"], 'plain', 'utf-8')
        msgAlternative.attach(msgText)
    if paramMap.has_key("html"):
        msgText = MIMEText(paramMap["html"], 'html', 'utf-8')
        msgAlternative.attach(msgText)
    if paramMap.has_key("image"):
        fp = open(paramMap["image"], 'rb')
        msgImage = MIMEImage(fp.read())
        fp.close()
        msgImage.add_header('Content-ID', '<image1>' )
        msgRoot.attach(msgImage)
    if paramMap.has_key("cc"):
        TempAddTo = paramMap["to"] + "," + paramMap["cc"]
    if TempAddTo.find(",") != -1:
        FinallyAdd = TempAddTo.split(",")
    else:
        FinallyAdd = TempAddTo

#构造附件
    if len(sys.argv) >= 7:
        fileName = sys.argv[6]#"./attachment/ppt.zip"#"Users/wansongHome/Downloads/Lesson_5_RegressionDiagnostics.pptx"
        basename = os.path.basename(fileName)
        if os.path.exists(fileName): #数据文件存在
#        print "attachment:"
            data = open(fileName, 'rb')
            att = MIMEText(data.read(), 'base64', 'gb2312')
            att["Content-Type"] = 'application/octet-stream'
            att["Content-Disposition"] = 'attachment; filename="%s"' % basename
            msgRoot.attach(att)
            data.close()
    smtp.sendmail(paramMap["from"], FinallyAdd, msgRoot.as_string())
    smtp.quit()
    return True

def process(toAddress):
    global mailDict
    mailDict["to"] = toAddress
    initMailConf()
    sendMail(mailDict)

if __name__ == "__main__":
    if len(sys.argv) >= 4:
        if sys.argv[3].find('@') == -1:
            filetos = open(sys.argv[3])
            to = filetos.readline()
            while to:
                process(to)
                to = filetos.readline()
                time.sleep(20)
            filetos.close()
        else:
            process(sys.argv[3])
