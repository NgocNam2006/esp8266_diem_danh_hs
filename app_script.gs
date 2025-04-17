function doGet(e) {
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("DanhSach");

    if (e.parameter.action === "getTime") {
      var now = new Date();
      return ContentService
        .createTextOutput(Utilities.formatDate(now, Session.getScriptTimeZone(), "yyyy-MM-dd HH:mm:ss"))
        .setMimeType(ContentService.MimeType.TEXT);
    }

    var rfid = e.parameter.rfid;
    if (!rfid) throw new Error("Thiếu tham số RFID");

    var data = sheet.getDataRange().getValues();
    for (var i = 1; i < data.length; i++) {
      if (data[i][0].toString().toUpperCase() === rfid.toUpperCase()) {
        return ContentService
          .createTextOutput(data[i][1])
          .setMimeType(ContentService.MimeType.TEXT);
      }
    }

    return ContentService
      .createTextOutput("Unknown")
      .setMimeType(ContentService.MimeType.TEXT);

  } catch (error) {
    return ContentService
      .createTextOutput("Lỗi: " + error.message)
      .setMimeType(ContentService.MimeType.TEXT);
  }
}

function doPost(e) {
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("DiemDanh");
    if (!sheet) throw new Error("Không tìm thấy sheet 'DiemDanh'");

    var data = JSON.parse(e.postData.contents);

    var rfid = data.rfid || "unknown";
    var fullName = data.name || "unknown";
    var now = new Date();

    sheet.appendRow([rfid, fullName, now]);

    return ContentService
      .createTextOutput("Đã lưu thành công")
      .setMimeType(ContentService.MimeType.TEXT);

  } catch (error) {
    return ContentService
      .createTextOutput("Lỗi: " + error.message)
      .setMimeType(ContentService.MimeType.TEXT);
  }
}
