function doGet(e) {
  try {
    var sheet = SpreadsheetApp.getActiveSpreadsheet().getSheetByName("DanhSach");

    if (e.parameter.action === "getTime") {
      var now = new Date();
      return ContentService
        .createTextOutput(Utilities.formatDate(now, "GMT+7", "yyyy-MM-dd HH:mm:ss"))
        .setMimeType(ContentService.MimeType.TEXT);
    }

    var rfid = e.parameter.rfid;
    if (!rfid) throw new Error("Thiếu tham số RFID");

    var data = sheet.getDataRange().getValues();
    for (var i = 1; i < data.length; i++) {
      if (data[i][0].toString().toUpperCase() === rfid.toUpperCase()) {
        var result = {
          name: data[i][1],
          class: data[i][2]
        };
        return ContentService
          .createTextOutput(JSON.stringify(result))
          .setMimeType(ContentService.MimeType.JSON);
      }
    }

    var unknown = {
      name: "Unknown",
      class: ""
    };
    return ContentService
      .createTextOutput(JSON.stringify(unknown))
      .setMimeType(ContentService.MimeType.JSON);

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
    var className = data.class || "";
    var now = new Date();

    sheet.appendRow([rfid, fullName, className, now]);

    return ContentService
      .createTextOutput("Đã lưu thành công")
      .setMimeType(ContentService.MimeType.TEXT);

  } catch (error) {
    return ContentService
      .createTextOutput("Lỗi: " + error.message)
      .setMimeType(ContentService.MimeType.TEXT);
  }
}
