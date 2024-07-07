const nodemailer = require('nodemailer');
const config_module = require("./conf")
/**
 * 创建发送邮件的代理
 */
let transport = nodemailer.createTransport({
    host: 'smtp.163.com',
    port: 465,
    secure: true,
    auth: {
        user: config_module.email_user, // 发送方邮箱地址
        pass: config_module.email_pass // 邮箱授权码或者密码
    }
});


/**
 * 发送邮件的函数
 * @param {*} mailOptions_ 发送邮件的参数,异步调用，promise类似于c++中的future
 * @returns 
 */
function SendMail(mailOptions_){
    return new Promise(function(resolve, reject){
        transport.sendMail(mailOptions_, function(error, info){
            if (error) {
                console.log(error);
                reject(error);
            } else {
                console.log('邮件已成功发送：' + info.response);
                resolve(info.response)
            }
        });
    })
   
}

module.exports.SendMail = SendMail
